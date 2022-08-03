from fileinput import lineno
import sys
import re
import subprocess
from enum import Enum
from typing import List, Tuple
from uuid import uuid4
from pathlib import Path

COMPILER_NAME = "peony"
COMPILER_DIAGNOSTIC_RE = re.compile(r"^((?P<file>.*?):(?P<lineno>\d+):((?P<colno>\d+):)? )?(?P<severity>(\w+)+):(?P<message>.*)$", re.MULTILINE)
ERROR_ANNOTATION_RE = re.compile(r"//~(?P<line_modifier>!|\^+|\|)?\s*(?P<severity>ERROR|WARNING|NOTE):(?P<message>.*)$", re.MULTILINE)
RUN_ANNOTATION_RE = re.compile(r"//#\s*RUN(\(\s*(?P<platform>\w+)\s*\))?:(?P<command>.*)$", re.MULTILINE)

def check_platform(platform: str) -> bool:
    """
    Returns true if the plaform string found in a RUN(platform) annotation
    corresponds to the current platform.
    """
    if platform is None or len(platform) == 0:
        return True
    return sys.platform.startswith(platform.lower())

# From https://stackoverflow.com/a/45142535/8959979
def finditer_with_line_numbers(pattern, string, flags=0):
    """
    A version of 're.finditer' that returns '(match, line_number)' pairs.
    Line numbers are 1-based.
    """
    matches = list(re.finditer(pattern, string, flags))
    if not matches:
        return []

    end = matches[-1].start()
    # -1 so a failed 'rfind' maps to the first line.
    newline_table = {-1: 0}
    for i, m in enumerate(re.finditer('\\n', string), 1):
        # Don't find newlines past our last match.
        offset = m.start()
        if offset > end:
            break
        newline_table[offset] = i

    # Failing to find the newline is OK, -1 maps to 0.
    for m in matches:
        newline_offset = string.rfind('\n', 0, m.start())
        line_number = newline_table[newline_offset]
        yield (m, line_number + 1)

def get_common_args_for_compiler(dir: Path) -> List[str]:
    if dir == dir.parent:
        return []

    args = get_common_args_for_compiler(dir.parent)

    file = (dir / "test_common.args")
    if file.exists():
        with open(file, 'r') as f:
            args += f.read().splitlines()

    return args

def add_debugger_to_cmd(cmd: List[str]) -> List[str]:
    if sys.platform.startswith("linux"):
        return ["gdb"] + cmd
    elif sys.platform.startswith("win32"):
        # call devenv
        return ["devenv", "/debugexe"] + cmd
    # unsupported platform, do not add debugger
    return cmd

class TestStatus(Enum):
    """
    The different possible outcome for a test.
    """
    PASS = "PASS"
    FAIL = "FAIL"
    XPASS = "XPASS"
    XFAIL = "XFAIL"
    TIMEOUT = "TIMEOUT"
    UNRESOLVED = "UNRESOLVED"

    def is_fail(self) -> bool:
        """Does this status represents a failed test or not?"""
        if self.value == "FAIL" or self.value == "TIMEOUT" or self.value == "UNRESOLVED":
            return True
        return False

class TestResult:
    def __init__(self, status: TestStatus, log: str) -> None:
        self.status = status
        self.log = log

    def is_fail(self) -> bool:
        """Does the test failed?"""
        return self.status.is_fail()

class Diagnostic:
    """
    Represents a diagnostic parsed from the compiler output.
    """
    def __init__(self, severity: str, msg: str, file: str|None, lineno: int|None, colno: int|None) -> None:
        severity = severity.lower().strip()
        assert((severity in ['note', 'warning', 'error']))
        self.severity = severity
        self.msg = msg.strip()
        self.file = file
        self.lineno = lineno
        self.colno = colno

    def __str__(self) -> str:
        lineno = self.lineno if self.lineno is not None else "?"
        colno = self.colno if self.colno is not None else "?"
        return "<source>:{}:{}: {}: {}".format(lineno, colno, self.severity, self.msg)

def parse_and_normalize_compiler_output(compiler_output: str) -> Tuple[List[Diagnostic], str]:
    """
    Generates a list of diagnostic from the given compiler output.
    """
    diagnostics = []
    patches = []
    for diagnostic in re.finditer(COMPILER_DIAGNOSTIC_RE, compiler_output):
        file = diagnostic.group('file')
        lineno = int(diagnostic.group('lineno')) if diagnostic.group('lineno') is not None else None
        colno = int(diagnostic.group('colno')) if diagnostic.group('colno') is not None else None
        severity = str(diagnostic.group('severity'))
        message = str(diagnostic.group('message'))
        diagnostics.append(Diagnostic(severity, message, file, lineno, colno))

        if file is not None:
            span = diagnostic.span('file')
            replace_txt = '<source>'
            patches.append((span, replace_txt))

    # Apply patches to normalize compiler output
    offset_correction = 0
    for patch in patches:
        start = patch[0][0] - offset_correction
        end = patch[0][1] - offset_correction
        compiler_output = "".join([compiler_output[:start], patch[1], compiler_output[end:]])
        offset_correction += (end - start) - len(patch[1])

    return diagnostics, compiler_output

def find_compiler_exe() -> str:
    exe_suffix = ""
    if sys.platform.startswith("win32"):
        exe_suffix = ".exe"

    dir = Path(".").absolute()
    if dir.name == "test":
        dir = dir.parent
    if (dir / "out").exists():
        dir = dir / "out"
        if (dir / "build" / "x86-debug" / (COMPILER_NAME + exe_suffix)).exists():
            return str(dir / "build" / "x86-debug" / (COMPILER_NAME + exe_suffix))
        if (dir / "build" / "x86-release" / (COMPILER_NAME + exe_suffix)).exists():
            return str(dir / "build" / "x86-release" / (COMPILER_NAME + exe_suffix))
        if (dir / "build" / "x64-debug" / (COMPILER_NAME + exe_suffix)).exists():
            return str(dir / "build" / "x64-debug" / (COMPILER_NAME + exe_suffix))
        if (dir / "build" / "x64-release" / (COMPILER_NAME + exe_suffix)).exists():
            return str(dir / "build" / "x64-release" / (COMPILER_NAME + exe_suffix))
    return None

class ExpectedDiagnostic:
    """
    Represents a error annototation (e.g. //~ ERROR: ...) parsed from the test file.
    """
    def __init__(self, severity: str, msg: str, lineno: int|None) -> None:
        severity = severity.lower().strip()
        assert(severity in ['note', 'warning', 'error'])
        self.severity = severity
        self.msg = msg.strip()
        self.lineno = lineno
        assert(lineno is None or lineno > 0)

    def match(self, compiler_diagnostic: Diagnostic) -> bool:
        """
        Checks if this expected diagnostic and the given compiler_diagnostic are compatible.
        """
        return self.lineno == compiler_diagnostic.lineno and self.severity == compiler_diagnostic.severity and self.msg == compiler_diagnostic.msg

    def __str__(self) -> str:
        lineno = self.lineno if self.lineno is not None else "?"
        return "<source>:{}:?: {}: {}".format(lineno, self.severity, self.msg)

class Test:
    """
    Represents a single test file that can contains many commands.
    """

    def __init__(self, file: str, name: str|None = None, tmp_dir: str|None = None, compiler_exe: str|None = None, debug: bool = False) -> None:
        self.file = Path(file).absolute()

        if name is None:
            self.name = self.file.stem
        else:
            self.name = name

        if tmp_dir is None:
            self.tmp_dir = Path.cwd()
        else:
            self.tmp_dir = Path(tmp_dir)

        if compiler_exe is not None:
            self.compiler_exe = compiler_exe
        else:
            self.compiler_exe = find_compiler_exe()

        self.debug = debug
        self.parse_annotations()

    def _generate_unique_filename(self) -> Path:
        return self.tmp_dir / ("tmp-{}-{}".format(self.name, uuid4()))

    def _preprocess_cmd(self, cmd: str) -> List[str]:
        cmd = cmd.strip()
        cmd = cmd.replace('%s', str(self.file))
        cmd = cmd.replace('%p', str(self.file))
        cmd = cmd.replace('%S', str(self.file.parent))
        cmd = cmd.replace('%t', str(self._generate_unique_filename()))

        splitted_cmd = cmd.split()
        if splitted_cmd[0] == COMPILER_NAME:
            if self.compiler_exe is not None:
                splitted_cmd[0] = self.compiler_exe

            args = get_common_args_for_compiler(self.file.parent)
            splitted_cmd = splitted_cmd[:1] + args + splitted_cmd[1:]

            if self.debug:
                splitted_cmd = add_debugger_to_cmd(splitted_cmd)

        return splitted_cmd

    def parse_annotations(self) -> None:
        self.run_commands = []
        self.expected_diagnostics = []
        self.expect_at_least_one_error = False
        with open(self.file, 'r') as f:
            content = str(f.read())

            for command_match in re.finditer(RUN_ANNOTATION_RE, content):
                platform = command_match.group("platform")
                command = command_match.group("command")
                if check_platform(platform):
                    command = self._preprocess_cmd(command)
                    self.run_commands.append(command)

            for error_match in finditer_with_line_numbers(ERROR_ANNOTATION_RE, content):
                match, lineno = error_match
                line_modifier = match.group("line_modifier")
                severity = match.group("severity")
                message = match.group("message")

                if line_modifier is None:
                    pass # do not change the line number
                elif line_modifier == '!':
                    lineno = None
                elif line_modifier == '|':
                    assert(len(self.expected_diagnostics) > 0 and self.expected_diagnostics[-1].lineno is not None)
                    lineno = self.expected_diagnostics[-1].lineno
                else: # "^" modifiers
                    lineno -= len(line_modifier)
                    assert(lineno >= 1)

                self.expected_diagnostics.append(ExpectedDiagnostic(severity, message, lineno))

        assert(len(self.run_commands) > 0) # what the interest to having an empty test?

    def get_stderr_filename(self, idx: int) -> str:
        return str(self.file.parent / ("".join([self.file.stem, ".", str(idx), ".stderr"])))

    def load_expected_stderr(self, idx: int) -> None:
        stderr_output_file = self.get_stderr_filename(idx)
        try:
            with open(stderr_output_file, 'r') as f:
                return str(f.read())
        except:
            return "" # no expected stderr output

    def run_command(self, idx: int, timeout: float|None = None, generate_stderr: bool = False) -> TestStatus:
        result = subprocess.run(self.run_commands[idx], shell=True, capture_output=True, timeout=timeout, encoding='utf-8')

        diagnostics, stderr = parse_and_normalize_compiler_output(str(result.stderr))
        has_errors = False
        for diagnostic in diagnostics:
            if diagnostic.severity == 'error':
                has_errors = True

        has_failed = False
        log = ""
        if result.returncode == 0 and has_errors:
            log += "FAIL: The compiler has returned 0 yet it has issued an error diagnosis\n"
            has_failed = True
        elif result.returncode != 0 and not has_errors:
            log += "FAIL: The compiler did not return 0 yet it did not emit any error diagnosis\n"
            has_failed = True

        if not generate_stderr:
            expected_stderr = self.load_expected_stderr(idx)
            if len(stderr) > 0 and len(expected_stderr) == 0:
                log += "FAIL: Unexpected output to stderr\n"
                log += "#> Compiler stderr output:\n"
                log += stderr
                has_failed = True
            elif stderr != expected_stderr:
                log += "FAIL: Mismatch between compiler output to stderr and expected output\n"
                log += "!> Expected stderr output:\n"
                log += expected_stderr
                log += "#> Compiler stderr output:\n"
                log += stderr
                has_failed = True

            for i in range(min(len(diagnostics), len(self.expected_diagnostics))):
                if not self.expected_diagnostics[i].match(diagnostics[i]):
                    log += "FAIL: Mismatch between compiler diagnostic and expected diagnostic\n"
                    log += "  !> Expected diagnostic:\n"
                    log += "      " + str(self.expected_diagnostics[i]) + "\n"
                    log += "  #> Compiler diagnostic:\n"
                    log += "      " + str(diagnostics[i]) + "\n"
                    has_failed = True

        for i in range(min(len(diagnostics), len(self.expected_diagnostics))):
            if not self.expected_diagnostics[i].match(diagnostics[i]):
                log += "FAIL: Mismatch between compiler diagnostic and expected diagnostic\n"
                log += "  !> Expected diagnostic:\n"
                log += "      " + str(self.expected_diagnostics[i]) + "\n"
                log += "  #> Compiler diagnostic:\n"
                log += "      " + str(diagnostics[i]) + "\n"
                has_failed = True

        if len(diagnostics) < len(self.expected_diagnostics):
            for i in range(len(diagnostics), len(self.expected_diagnostics)):
                log += "FAIL: Expected diagnostic not found in compiler output\n"
                log += "  !> Expected diagnostic:\n"
                log += "      " + str(self.expected_diagnostics[i]) + "\n"
                has_failed = True
        elif len(diagnostics) > len(self.expected_diagnostics):
            for i in range(len(self.expected_diagnostics), len(diagnostics)):
                log += "FAIL: Unexpected diagnostic from compiler output\n"
                log += "  #> Compiler diagnostic:\n"
                log += "      " + str(diagnostics[i]) + "\n"
                has_failed = True

        if has_failed:
            log += "The command: " + " ".join(self.run_commands[idx]) + "\n"

        if len(log) > 0 and log[-1] == '\n':
            log = log[:-1]

        if has_failed:
            return TestResult(TestStatus.FAIL, log)
        else:
            if generate_stderr:
                stderr_output_file = self.get_stderr_filename(idx)
                with open(stderr_output_file, 'w') as f:
                    f.write(stderr)
            return TestResult(TestStatus.PASS, log)

    def run(self, timeout: float|None = None) -> int:
        exit_code = 0
        for i in range(len(self.run_commands)):
            result = self.run_command(i, timeout)
            print("{}: '{}' ({} of {})".format(result.status.value, " ".join(self.run_commands[i]), i + 1, len(self.run_commands)))
            if result.is_fail():
                if result.log is not None and len(result.log) > 0:
                    header = "{0} FAIL LOG {0}".format("*" * 15)
                    print(header)
                    print(result.log)
                    print("*" * len(header))
                exit_code = 1
        return exit_code

    def generate_stderr(self) -> int:
        exit_code = 0
        for i in range(len(self.run_commands)):
            result = self.run_command(i, generate_stderr=True)
            if result.is_fail():
                header = "{0} FAIL to generate STDERR for {1} cmd#{2} {0}".format("*" * 5, self.name, i)
                print(header)
                print(result.log)
                print("*" * len(header))
                exit_code = 1
            else:
                print("STDERR generated for {} cmd#{}".format(self.name, i))
        return exit_code

import argparse

def main():
    parser = argparse.ArgumentParser(description='{} test suit driver.'.format(COMPILER_NAME))
    parser.add_argument("--timeout", type=int, default=5, help="max time, in seconds, that we must wait for a test to finish")
    parser.add_argument("--compiler-exe", type=str, help="path to the compiler executable; all RUN commands calling the compiler will be modified to use this path")
    parser.add_argument("--file", type=str, required=True, help="path to the test file to execute")
    parser.add_argument("--name", type=str, help="name of the test (default is test file name)")
    parser.add_argument("--tmp-dir", type=str, help="a temporary directory that can be used by some tests (default is cwd)")
    parser.add_argument("--generate-stderr", action="store_const", const=True, default=False, help="run the test and generates stderr files with the compiler output")
    parser.add_argument("--debug", action="store_const", const=True, default=False, help="run the compiler using the platform debugger")
    args = parser.parse_args()

    test = Test(args.file, name=args.name, compiler_exe=args.compiler_exe, tmp_dir=args.tmp_dir, debug=args.debug)
    if args.generate_stderr == True:
        return test.generate_stderr()
    else:
        return test.run(args.timeout)

if __name__ == "__main__":
    exit_code = main()
    sys.exit(exit_code)
