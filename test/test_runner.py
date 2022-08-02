import sys
import re
import subprocess
from enum import Enum
from uuid import uuid4
from pathlib import Path

class TestStatus(Enum):
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

class Test:
    def __init__(self, filepath: str) -> None:
        self.filepath = Path(filepath).absolute()
        self.fail_reason = str()

    def _generate_unique_filename(self) -> str:
        return "tmp-{}-{}".format(self.name, uuid4())

    def _preprocess_cmd(self, cmd: str) -> str:
        cmd = cmd.replace('%s', str(self.filepath))
        cmd = cmd.replace('%p', str(self.filepath))
        cmd = cmd.replace('%S', str(self.filepath.parent))
        cmd = cmd.replace('%t', self._generate_unique_filename())
        return cmd

    def parse_commands(self) -> None:
        """
        Parses RUN commands in the test file.
        """
        command_regex = re.compile(r'RUN:(.*)')
        self.commands = []
        with open(self.filepath, 'r') as f:
            content = f.read()
            for command in command_regex.findall(content):
                command = command.strip()
                command = self._preprocess_cmd(command)
                self.commands.append(command)

    def set_name(self, name: str|None) -> None:
        if name is None:
            self.name = self.filepath.stem
        else:
            self.name = name

    def set_peony_exe(self, path: str|None) -> None:
        if path is None:
            return

        for i, command in enumerate(self.commands):
            if command.startswith("peony "):
                self.commands[i] = path + command[5:]

    def run(self, timeout: int) -> int:
        exit_code = 0
        for i, command in enumerate(self.commands):
            splitted_cmd = command.split()
            try:
                subprocess.run(splitted_cmd, check=True, timeout=timeout)
                status = TestStatus.PASS
            except subprocess.CalledProcessError as e:
                self.fail_reason = "Test '{}' failed because of its exit code {}".format(self.name, e.returncode)
                status = TestStatus.FAIL
            except subprocess.TimeoutExpired:
                self.fail_reason = "Test '{}' failed because of timeout (maybe infinite loop?)".format(self.name)
                status = TestStatus.TIMEOUT
            except FileNotFoundError:
                self.fail_reason = "Could not launch test '{}'".format(self.name)
                status = TestStatus.UNRESOLVED
            print("{}: {} ({} of {})".format(status.name, self.name, i + 1, len(self.commands)))
            if status.is_fail():
                self.print_fail_details(i, splitted_cmd if status == TestStatus.UNRESOLVED else None)
                exit_code = 1
        return exit_code

    def print_fail_details(self, command_i: int, command) -> None:
        header = "{0} COMMAND {1} of {2} {0}".format("*" * 18, command_i, self.name)
        print(header)
        print(self.fail_reason)
        if command is not None:
            print("The command:", command)
        print("*" * len(header))

import argparse

def main():
    parser = argparse.ArgumentParser(description='Peony test suit driver.')
    parser.add_argument("--timeout", type=int, default=5, help="max time, in seconds, that we must wait for a test to finish")
    parser.add_argument("--peony-exe", type=str, help="path to the peony executable; all RUN commands calling peony will be modified to use this path")
    parser.add_argument("--file", type=str, required=True, help="path to the test file to execute")
    parser.add_argument("--name", type=str, help="name of the test, if omitted then the file name is taken by default")
    args = parser.parse_args()

    test = Test(args.file)
    test.set_name(args.name)
    test.parse_commands()
    test.set_peony_exe(args.peony_exe)
    return test.run(args.timeout)

if __name__ == "__main__":
    exit_code = main()
    sys.exit(exit_code)
