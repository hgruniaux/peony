#ifndef PEONY_INTERPRETER_VALUE_HXX
#define PEONY_INTERPRETER_VALUE_HXX

#include <cstdint>

class PInterpreterValue
{
public:
  enum class Kind
  {
    None,
    Indeterminate,
    Bool,
    Integer,
    Float,
  };

  PInterpreterValue();

  static PInterpreterValue make_none() { return {}; }
  static PInterpreterValue make_indeterminate();
  static PInterpreterValue make_bool(bool p_value);
  static PInterpreterValue make_integer(intmax_t p_value);
  static PInterpreterValue make_float(double p_value);

  [[nodiscard]] Kind get_kind() const { return m_kind; }

  [[nodiscard]] bool is_none() const { return get_kind() == Kind::None; }
  [[nodiscard]] bool is_indeterminate() const { return get_kind() == Kind::Indeterminate; }
  [[nodiscard]] bool is_bool() const { return get_kind() == Kind::Bool; }
  [[nodiscard]] bool is_integer() const { return get_kind() == Kind::Integer; }
  [[nodiscard]] bool is_float() const { return get_kind() == Kind::Float; }

  void set_none() { m_kind = Kind::None; }
  void set_indeterminate() { m_kind = Kind::Indeterminate; }
  void set_bool(bool p_value);
  void set_integer(intmax_t p_value);
  void set_float(double p_value);

  [[nodiscard]] bool get_bool() const;
  [[nodiscard]] intmax_t get_integer() const;
  [[nodiscard]] double get_float() const;

  void into_bool();
  void into_integer();
  void into_float();

  PInterpreterValue& operator+=(const PInterpreterValue& p_other);
  PInterpreterValue& operator-=(const PInterpreterValue& p_other);
  PInterpreterValue& operator*=(const PInterpreterValue& p_other);
  PInterpreterValue& operator/=(const PInterpreterValue& p_other);
  PInterpreterValue& operator%=(const PInterpreterValue& p_other);
  PInterpreterValue& operator<<=(const PInterpreterValue& p_other);
  PInterpreterValue& operator>>=(const PInterpreterValue& p_other);
  PInterpreterValue& operator&=(const PInterpreterValue& p_other);
  PInterpreterValue& operator|=(const PInterpreterValue& p_other);
  PInterpreterValue& operator^=(const PInterpreterValue& p_other);

  PInterpreterValue operator!();
  PInterpreterValue operator+() { return *this; }
  PInterpreterValue operator-();
  PInterpreterValue operator~();

  bool operator==(const PInterpreterValue& p_other) const;

private:
  Kind m_kind;
  union
  {
    bool m_bool_value;
    intmax_t m_integer_value;
    double m_float_value;
  };
};

#endif // PEONY_INTERPRETER_VALUE_HXX
