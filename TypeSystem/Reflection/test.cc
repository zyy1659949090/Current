/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Maxim Zhurovich <zhurovich@gmail.com>
          (c) 2015 Dmitry "Dima" Korolev <dmitry.korolev@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

// This `test.cc` file is `#include`-d from `../test.cc`, and thus needs a header guard.

#ifndef CURRENT_TYPE_SYSTEM_REFLECTION_TEST_CC
#define CURRENT_TYPE_SYSTEM_REFLECTION_TEST_CC

#include <cstdint>

#include "reflection.h"
#include "schema.h"

#include "../../Bricks/strings/strings.h"
#include "../../Bricks/file/file.h"
#include "../../Bricks/dflags/dflags.h"

#include "../../3rdparty/gtest/gtest-main-with-dflags.h"

DEFINE_bool(write_reflection_golden_files, false, "Set to true to [over]write the golden files.");

namespace reflection_test {

// A few properly defined Current data types.
CURRENT_STRUCT(Foo) { CURRENT_FIELD(i, uint64_t, 42u); };
CURRENT_STRUCT(Bar) {
  CURRENT_FIELD(v1, std::vector<uint64_t>);
  CURRENT_FIELD(v2, std::vector<Foo>);
  CURRENT_FIELD(v3, std::vector<std::vector<Foo>>);
  CURRENT_FIELD(v4, (std::map<std::string, std::string>));
};
CURRENT_STRUCT(DerivedFromFoo, Foo) { CURRENT_FIELD(bar, Bar); };
CURRENT_STRUCT(SelfContainingA) { CURRENT_FIELD(v, std::vector<SelfContainingA>); };
CURRENT_STRUCT(SelfContainingB) { CURRENT_FIELD(v, std::vector<SelfContainingB>); };
CURRENT_STRUCT(SelfContainingC, SelfContainingA) {
  CURRENT_FIELD(v, std::vector<SelfContainingB>);
  CURRENT_FIELD(m, (std::map<std::string, SelfContainingC>));
};

using current::reflection::Reflector;

}  // namespace reflection_test

TEST(Reflection, TypeID) {
  using namespace reflection_test;
  using current::reflection::ReflectedType_Struct;

  const ReflectedType_Struct& bar = Value<ReflectedType_Struct>(Reflector().ReflectType<Bar>());
  EXPECT_EQ(4u, bar.fields.size());
  EXPECT_EQ(9319767778871345347ull, static_cast<uint64_t>(bar.fields[0].first));
  EXPECT_EQ(9319865771553050731ull, static_cast<uint64_t>(bar.fields[1].first));
  EXPECT_EQ(9311949877586199388ull, static_cast<uint64_t>(bar.fields[2].first));
  EXPECT_EQ(9349351407460177576ull, static_cast<uint64_t>(bar.fields[3].first));
  EXPECT_EQ(bar.type_id, Value<ReflectedType_Struct>(Reflector().ReflectedTypeByTypeID(bar.type_id)).type_id);

  const ReflectedType_Struct& self_a = Value<ReflectedType_Struct>(Reflector().ReflectType<SelfContainingA>());
  EXPECT_EQ(1u, self_a.fields.size());
  EXPECT_EQ(9205901389225534299ull, static_cast<uint64_t>(self_a.type_id));
  EXPECT_EQ(9317324759808216579ull, static_cast<uint64_t>(self_a.fields[0].first));
  const ReflectedType_Struct& self_b = Value<ReflectedType_Struct>(Reflector().ReflectType<SelfContainingB>());
  EXPECT_EQ(1u, self_b.fields.size());
  EXPECT_EQ(9203772139816579809ull, static_cast<uint64_t>(self_b.type_id));
  EXPECT_EQ(9317324775776617427ull, static_cast<uint64_t>(self_b.fields[0].first));
  const ReflectedType_Struct& self_c = Value<ReflectedType_Struct>(Reflector().ReflectType<SelfContainingC>());
  EXPECT_EQ(2u, self_c.fields.size());
  EXPECT_EQ(9200564679597442224ull, static_cast<uint64_t>(self_c.type_id));
  EXPECT_EQ(9317324775776617427ull, static_cast<uint64_t>(self_c.fields[0].first));
  EXPECT_EQ(9345487227046290999ull, static_cast<uint64_t>(self_c.fields[1].first));
}

TEST(Reflection, CurrentStructInternals) {
  using namespace reflection_test;
  using namespace current::reflection;

  static_assert(std::is_same<SuperType<Foo>, ::current::CurrentSuper>::value, "");
  EXPECT_EQ(1u, FieldCounter<Foo>::value);

  Foo::CURRENT_REFLECTION([](TypeSelector<uint64_t>, const std::string& name) { EXPECT_EQ("i", name); },
                          Index<FieldTypeAndName, 0>());

  Foo foo;
  foo.i = 100u;
  foo.CURRENT_REFLECTION([](const std::string& name, const uint64_t& value) {
    EXPECT_EQ("i", name);
    EXPECT_EQ(100u, value);
  }, Index<FieldNameAndImmutableValue, 0>());

  foo.CURRENT_REFLECTION([](const std::string& name, uint64_t& value) {
    EXPECT_EQ("i", name);
    value = 123u;
  }, Index<FieldNameAndMutableValue, 0>());
  EXPECT_EQ(123u, foo.i);

  static_assert(std::is_same<SuperType<Bar>, ::current::CurrentSuper>::value, "");
  EXPECT_EQ(4u, FieldCounter<Bar>::value);
  static_assert(std::is_same<SuperType<DerivedFromFoo>, Foo>::value, "");
  EXPECT_EQ(1u, FieldCounter<DerivedFromFoo>::value);
}

namespace reflection_test {

CURRENT_ENUM(Enum, uint32_t){Value1 = 1u, Value2 = 2u};

CURRENT_STRUCT(StructWithAllSupportedTypes) {
  // Integral.
  CURRENT_FIELD(b, bool, true);
  CURRENT_FIELD(c, char, 'Q');
  CURRENT_FIELD(uint8, uint8_t, UINT8_MAX);
  CURRENT_FIELD(uint16, uint16_t, UINT16_MAX);
  CURRENT_FIELD(uint32, uint32_t, UINT32_MAX);
  CURRENT_FIELD(uint64, uint64_t, UINT64_MAX);
  CURRENT_FIELD(int8, int8_t, INT8_MIN);
  CURRENT_FIELD(int16, int16_t, INT16_MIN);
  CURRENT_FIELD(int32, int32_t, INT32_MIN);
  CURRENT_FIELD(int64, int64_t, INT64_MIN);
  // Floating point.
  CURRENT_FIELD(flt, float, 1e38);
  CURRENT_FIELD(dbl, double, 1e308);
  // Other primitive types.
  CURRENT_FIELD(s, std::string, "The String");
  CURRENT_FIELD(e, Enum, Enum::Value2);
  // STL containers.
  CURRENT_FIELD(pair_strdbl, (std::pair<std::string, double>));
  CURRENT_FIELD(vector_int32, std::vector<int32_t>);
  CURRENT_FIELD(map_strstr, (std::map<std::string, std::string>));
  // Current complex types.
  CURRENT_FIELD(optional_i, Optional<int32_t>);
  CURRENT_FIELD(optional_b, Optional<bool>);
};
}

namespace reflection_test {

struct CollectFieldValues {
  std::vector<std::string>& output_;

  template <typename T>
  ENABLE_IF<!std::is_enum<T>::value> operator()(const std::string&, const T& value) const {
    output_.push_back(current::strings::ToString(value));
  }

  template <typename T>
  ENABLE_IF<std::is_enum<T>::value> operator()(const std::string&, const T& value) const {
    output_.push_back(current::strings::ToString(static_cast<typename std::underlying_type<T>::type>(value)));
  }

  template <typename T>
  void operator()(const std::string&, const std::vector<T>& value) const {
    output_.push_back('[' + current::strings::Join(value, ',') + ']');
  }

  template <typename TF, typename TS>
  void operator()(const std::string&, const std::pair<TF, TS>& value) const {
    output_.push_back(current::strings::ToString(value.first) + ':' + current::strings::ToString(value.second));
  }

  template <typename TK, typename TV>
  void operator()(const std::string&, const std::map<TK, TV>& value) const {
    std::ostringstream oss;
    oss << '[';
    bool first = true;
    for (const auto& cit : value) {
      if (first) {
        first = false;
      } else {
        oss << ',';
      }
      oss << cit.first << ':' << cit.second;
    }
    oss << ']';
    output_.push_back(oss.str());
  }

  // Output `bool` using boolalpha.
  void operator()(const std::string&, bool value) const {
    std::ostringstream oss;
    oss << std::boolalpha << value;
    output_.push_back(oss.str());
  }

  // Output floating types in scientific notation.
  void operator()(const std::string&, float value) const {
    std::ostringstream oss;
    oss << value;
    output_.push_back(oss.str());
  }

  void operator()(const std::string&, double value) const {
    std::ostringstream oss;
    oss << value;
    output_.push_back(oss.str());
  }

  // Output `Optional`.
  template <typename T>
  void operator()(const std::string&, const Optional<T>& value) const {
    std::ostringstream oss;
    if (Exists(value)) {
      oss << Value(value);
    } else {
      oss << "null";
    }
    output_.push_back(oss.str());
  }
};

}  // namespace reflection_test

TEST(Reflection, VisitAllFields) {
  using namespace reflection_test;

  StructWithAllSupportedTypes all;
  all.pair_strdbl = {"Minus eight point five", -9.5};
  all.vector_int32 = {-1, -2, -4};
  all.map_strstr = {{"key1", "value1"}, {"key2", "value2"}};
  all.optional_i = 128;  // Leaving `optional_b` empty.

  std::vector<std::string> result;
  CollectFieldValues values{result};
  current::reflection::VisitAllFields<StructWithAllSupportedTypes,
                                      current::reflection::FieldNameAndImmutableValue>::WithObject(all, values);
  EXPECT_EQ(
      "true,"
      "Q,"
      "255,65535,4294967295,18446744073709551615,"
      "-128,-32768,-2147483648,-9223372036854775808,"
      "1e+38,1e+308,"
      "The String,"
      "2,"
      "Minus eight point five:-9.500000,"
      "[-1,-2,-4],"
      "[key1:value1,key2:value2],"
      "128,null",
      current::strings::Join(result, ','));
}

namespace reflection_test {

CURRENT_STRUCT(X) { CURRENT_FIELD(i, int32_t); };
CURRENT_STRUCT(Y) { CURRENT_FIELD(v, std::vector<X>); };
CURRENT_STRUCT(Z, Y) {
  CURRENT_FIELD(d, double);
  CURRENT_FIELD(v2, std::vector<std::vector<Enum>>);
};

CURRENT_STRUCT(A) { CURRENT_FIELD(i, uint32_t); };
CURRENT_STRUCT(B) {
  CURRENT_FIELD(x, X);
  CURRENT_FIELD(a, A);
};
CURRENT_STRUCT(C) { CURRENT_FIELD(b, Optional<B>); };
}

TEST(Reflection, StructSchema) {
  using namespace reflection_test;
  using current::reflection::SchemaInfo;
  using current::reflection::StructSchema;
  using current::reflection::Language;

  StructSchema struct_schema;
#if 0
  {
    const SchemaInfo schema = struct_schema.GetSchemaInfo();
    EXPECT_TRUE(schema.order.empty());
    EXPECT_TRUE(schema.types.empty());
    EXPECT_EQ("", struct_schema.Describe(Language::CPP(), false));
  }

  struct_schema.AddType<uint64_t>();
  struct_schema.AddType<double>();
  struct_schema.AddType<std::string>();

  {
    const SchemaInfo schema = struct_schema.GetSchemaInfo();
    EXPECT_TRUE(schema.order.empty());
    EXPECT_TRUE(schema.types.empty());
    EXPECT_EQ("", struct_schema.Describe(Language::CPP(), false));
  }
#endif
  struct_schema.AddType<Z>();

  {
    EXPECT_EQ(
        "struct X {\n"
        "  int32_t i;\n"
        "};\n"
        "struct Y {\n"
        "  std::vector<X> v;\n"
        "};\n"
        "struct Z : Y {\n"
        "  double d;\n"
        "  std::vector<std::vector<Enum>> v2;\n"
        "};\n",
        struct_schema.Describe(Language::CPP(), false));
  }

  struct_schema.AddType<C>();

  {
    EXPECT_EQ(
        "struct X {\n"
        "  int32_t i;\n"
        "};\n"
        "struct Y {\n"
        "  std::vector<X> v;\n"
        "};\n"
        "struct Z : Y {\n"
        "  double d;\n"
        "  std::vector<std::vector<Enum>> v2;\n"
        "};\n"
        "struct A {\n"
        "  uint32_t i;\n"
        "};\n"
        "struct B {\n"
        "  X x;\n"
        "  A a;\n"
        "};\n"
        "struct C {\n"
        "  Optional<B> b;\n"
        "};\n",
        struct_schema.Describe(Language::CPP(), false));
  }
}

#if 0
TEST(Reflection, SelfContatiningStruct) {
  using namespace reflection_test;
  using current::reflection::StructSchema;
  using current::reflection::Language;

  StructSchema struct_schema;
  struct_schema.AddType<SelfContainingC>();

  EXPECT_EQ(
      "struct SelfContainingA {\n"
      "  std::vector<SelfContainingA> v;\n"
      "};\n"
      "struct SelfContainingB {\n"
      "  std::vector<SelfContainingB> v;\n"
      "};\n"
      "struct SelfContainingC : SelfContainingA {\n"
      "  std::vector<SelfContainingB> v;\n"
      "  std::map<std::string, SelfContainingC> m;\n"
      "};\n",
      struct_schema.Describe(Language::CPP(), false));
}

#include "../Serialization/json.h"

#define SMOKE_TEST_STRUCT_NAMESPACE smoke_test_struct_namespace
#include "smoke_test_struct.h"
#undef SMOKE_TEST_STRUCT_NAMESPACE

TEST(Reflection, SmokeTestFullStruct) {
  using current::FileSystem;
  using current::reflection::StructSchema;
  using current::reflection::SchemaInfo;
  using current::reflection::Language;

  StructSchema struct_schema;
  struct_schema.AddType<smoke_test_struct_namespace::FullTest>();

  if (false) {
    // LCOV_EXCL_START
    // This will not run, but should compile.
    smoke_test_struct_namespace::FullTest original =
        current::FromIncomplete<smoke_test_struct_namespace::FullTest>(
            current::Incomplete<smoke_test_struct_namespace::FullTest>());
    smoke_test_struct_namespace::FullTest moved(std::move(original));
    // TODO(dkorolev): These would not even compile yet.
    // Best idea I have in mind so far is for `Clone()` to create a blank result as `Incomplete<T>`,
    // and then copy it field-by-field via reflection and calling `Clone()` recursively.
    // smoke_test_struct_namespace::FullTest cloned(current::Clone(moved));
    // original = Clone(cloned);
    // LCOV_EXCL_STOP
  }
  if (FLAGS_write_reflection_golden_files) {
    // LCOV_EXCL_START
    FileSystem::WriteStringToFile(struct_schema.Describe(Language::CPP()), "golden/smoke_test_struct.cc");
    FileSystem::WriteStringToFile(struct_schema.Describe(Language::FSharp()), "golden/smoke_test_struct.fsx");
    FileSystem::WriteStringToFile(JSON(struct_schema.GetSchemaInfo()), "golden/smoke_test_struct.json");
    // LCOV_EXCL_STOP
  }

  EXPECT_EQ(FileSystem::ReadFileAsString("golden/smoke_test_struct.cc"),
            struct_schema.Describe(Language::CPP()));

  EXPECT_EQ(FileSystem::ReadFileAsString("golden/smoke_test_struct.fsx"),
            struct_schema.Describe(Language::FSharp()));

  // JSON is a special case, as it might be pretty-printed.
  EXPECT_EQ(JSON(ParseJSON<SchemaInfo>(FileSystem::ReadFileAsString("golden/smoke_test_struct.json"))),
            JSON(struct_schema.GetSchemaInfo()));
}
#endif

#endif  // CURRENT_TYPE_SYSTEM_REFLECTION_TEST_CC