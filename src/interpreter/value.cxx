#include "value.hxx"

#include <cassert>
#include <cmath>

PInterpreterValue::PInterpreterValue()
  : m_kind(Kind::None)
{
}

PInterpreterValue
PInterpreterValue::make_indeterminate()
{
  auto value = PInterpreterValue();
  value.m_kind = Kind::Indeterminate;
  return value;
}

PInterpreterValue
PInterpreterValue::make_bool(bool p_value)
{
  auto value = PInterpreterValue();
  value.m_kind = Kind::Bool;
  value.m_bool_value = p_value;
  return value;
}

PInterpreterValue
PInterpreterValue::make_integer(intmax_t p_value)
{
  auto value = PInterpreterValue();
  value.m_kind = Kind::Integer;
  value.m_integer_value = p_value;
  return value;
}

PInterpreterValue
PInterpreterValue::make_float(double p_value)
{
  auto value = PInterpreterValue();
  value.m_kind = Kind::Float;
  value.m_float_value = p_value;
  return value;
}

void
PInterpreterValue::set_bool(bool p_value)
{
  m_kind = Kind::Bool;
  m_bool_value = p_value;
}

void
PInterpreterValue::set_integer(intmax_t p_value)
{
  m_kind = Kind::Integer;
  m_integer_value = p_value;
}

void
PInterpreterValue::set_float(double p_value)
{
  m_kind = Kind::Float;
  m_float_value = p_value;
}

bool
PInterpreterValue::get_bool() const
{
  assert(is_bool());
  return m_bool_value;
}

intmax_t
PInterpreterValue::get_integer() const
{
  assert(is_integer());
  return m_integer_value;
}

double
PInterpreterValue::get_float() const
{
  assert(is_float());
  return m_float_value;
}

void
PInterpreterValue::into_bool()
{
  switch (m_kind) {
    case Kind::Bool:
      break;
    case Kind::Integer:
      set_bool(m_integer_value != 0);
      break;
    case Kind::Float:
      set_bool(m_float_value != 0);
      break;
    default:
      set_indeterminate();
      break;
  }
}

void
PInterpreterValue::into_integer()
{
  switch (m_kind) {
    case Kind::Bool:
      set_integer(m_bool_value);
    case Kind::Integer:
      break;
    case Kind::Float:
      set_integer(static_cast<intmax_t>(m_float_value));
      break;
    default:
      set_indeterminate();
      break;
  }
}

void
PInterpreterValue::into_float()
{
  switch (m_kind) {
    case Kind::Bool:
      set_float(m_bool_value);
      break;
    case Kind::Integer:
      set_float(static_cast<double>(m_integer_value));
      break;
    case Kind::Float:
      break;
    default:
      set_indeterminate();
      break;
  }
}

PInterpreterValue&
PInterpreterValue::operator+=(const PInterpreterValue& p_other)
{
  if (m_kind != p_other.m_kind) {
    set_indeterminate();
    return *this;
  }

  switch (m_kind) {
    case Kind::Integer:
      m_integer_value += p_other.m_integer_value;
      break;
    case Kind::Float:
      m_float_value += p_other.m_float_value;
      break;
    default:
      set_indeterminate();
      break;
  }

  return *this;
}

PInterpreterValue&
PInterpreterValue::operator-=(const PInterpreterValue& p_other)
{
  if (m_kind != p_other.m_kind) {
    set_indeterminate();
    return *this;
  }

  switch (m_kind) {
    case Kind::Integer:
      m_integer_value -= p_other.m_integer_value;
      break;
    case Kind::Float:
      m_float_value -= p_other.m_float_value;
      break;
    default:
      set_indeterminate();
      break;
  }

  return *this;
}

PInterpreterValue&
PInterpreterValue::operator*=(const PInterpreterValue& p_other)
{
  if (m_kind != p_other.m_kind) {
    set_indeterminate();
    return *this;
  }

  switch (m_kind) {
    case Kind::Integer:
      m_integer_value *= p_other.m_integer_value;
      break;
    case Kind::Float:
      m_float_value *= p_other.m_float_value;
      break;
    default:
      set_indeterminate();
      break;
  }

  return *this;
}

PInterpreterValue&
PInterpreterValue::operator/=(const PInterpreterValue& p_other)
{
  if (m_kind != p_other.m_kind) {
    set_indeterminate();
    return *this;
  }

  switch (m_kind) {
    case Kind::Integer:
      m_integer_value /= p_other.m_integer_value;
      break;
    case Kind::Float:
      m_float_value /= p_other.m_float_value;
      break;
    default:
      set_indeterminate();
      break;
  }

  return *this;
}

PInterpreterValue&
PInterpreterValue::operator%=(const PInterpreterValue& p_other)
{
  if (m_kind != p_other.m_kind) {
    set_indeterminate();
    return *this;
  }

  switch (m_kind) {
    case Kind::Integer:
      m_integer_value %= p_other.m_integer_value;
      break;
    case Kind::Float:
      m_float_value = std::fmod(m_float_value, p_other.m_float_value);
      break;
    default:
      set_indeterminate();
      break;
  }

  return *this;
}

PInterpreterValue&
PInterpreterValue::operator<<=(const PInterpreterValue& p_other)
{
  if (m_kind != p_other.m_kind) {
    set_indeterminate();
    return *this;
  }

  switch (m_kind) {
    case Kind::Integer:
      m_integer_value <<= p_other.m_integer_value;
      break;
    default:
      set_indeterminate();
      break;
  }

  return *this;
}

PInterpreterValue&
PInterpreterValue::operator>>=(const PInterpreterValue& p_other)
{
  if (m_kind != p_other.m_kind) {
    set_indeterminate();
    return *this;
  }

  switch (m_kind) {
    case Kind::Integer:
      m_integer_value >>= p_other.m_integer_value;
      break;
    default:
      set_indeterminate();
      break;
  }

  return *this;
}

PInterpreterValue&
PInterpreterValue::operator&=(const PInterpreterValue& p_other)
{
  if (m_kind != p_other.m_kind) {
    set_indeterminate();
    return *this;
  }

  switch (m_kind) {
    case Kind::Integer:
      m_integer_value &= p_other.m_integer_value;
      break;
    default:
      set_indeterminate();
      break;
  }

  return *this;
}

PInterpreterValue&
PInterpreterValue::operator|=(const PInterpreterValue& p_other)
{
  if (m_kind != p_other.m_kind) {
    set_indeterminate();
    return *this;
  }

  switch (m_kind) {
    case Kind::Integer:
      m_integer_value |= p_other.m_integer_value;
      break;
    default:
      set_indeterminate();
      break;
  }

  return *this;
}

PInterpreterValue&
PInterpreterValue::operator^=(const PInterpreterValue& p_other)
{
  if (m_kind != p_other.m_kind) {
    set_indeterminate();
    return *this;
  }

  switch (m_kind) {
    case Kind::Integer:
      m_integer_value ^= p_other.m_integer_value;
      break;
    default:
      set_indeterminate();
      break;
  }

  return *this;
}

PInterpreterValue
PInterpreterValue::operator!()
{
  switch (m_kind) {
    case Kind::Bool:
      return PInterpreterValue::make_bool(!m_bool_value);
    default:
      return PInterpreterValue::make_indeterminate();
  }
}

PInterpreterValue
PInterpreterValue::operator-()
{
  switch (m_kind) {
    case Kind::Integer:
      return PInterpreterValue::make_integer(-m_integer_value);
    case Kind::Float:
      return PInterpreterValue::make_float(-m_float_value);
    default:
      return PInterpreterValue::make_indeterminate();
  }
}

PInterpreterValue
PInterpreterValue::operator~()
{
  switch (m_kind) {
    case Kind::Integer:
      return PInterpreterValue::make_integer(~m_bool_value);
    default:
      return PInterpreterValue::make_indeterminate();
  }
}

bool
PInterpreterValue::operator==(const PInterpreterValue& p_other) const
{
  if (m_kind != p_other.m_kind)
    return false;

  switch (m_kind) {
    case Kind::Bool:
      return m_bool_value == p_other.m_bool_value;
    case Kind::Integer:
      return m_integer_value == p_other.m_integer_value;
    case Kind::Float:
      return m_float_value == p_other.m_float_value;
    default:
      return true;
  }
}
