/*

Copyright (c) 2013-2020, 2022, Arvid Norberg
Copyright (c) 2015, Mikhail Titov
Copyright (c) 2016, 2018, 2020, Alden Torres
Copyright (c) 2016, Andrei Kurushin
Copyright (c) 2016, Steven Siloti
Copyright (c) 2017, Pavel Pimenov
All rights reserved.

You may use, distribute and modify this code under the terms of the BSD license,
see LICENSE file.
*/

#include "test.hpp"
#include "libtorrent/aux_/escape_string.hpp"
#include "libtorrent/hex.hpp"
#include "libtorrent/aux_/string_util.hpp"
#include "libtorrent/aux_/string_ptr.hpp"
#include <iostream>
#include <cstring> // for strcmp
#include "libtorrent/aux_/escape_string.hpp" // for trim

using namespace lt;
using namespace lt::aux;

TORRENT_TEST(maybe_url_encode)
{
	// test maybe_url_encode
	TEST_EQUAL(maybe_url_encode("http://test:test@abc.com/abc<>abc"), "http://test:test@abc.com/abc%3c%3eabc");
	TEST_EQUAL(maybe_url_encode("http://abc.com/foo bar"), "http://abc.com/foo%20bar");
	TEST_EQUAL(maybe_url_encode("http://abc.com:80/foo bar"), "http://abc.com:80/foo%20bar");
	TEST_EQUAL(maybe_url_encode("http://abc.com:8080/foo bar"), "http://abc.com:8080/foo%20bar");
	TEST_EQUAL(maybe_url_encode("abc"), "abc");
	TEST_EQUAL(maybe_url_encode("http://abc.com/abc"), "http://abc.com/abc");
}

TORRENT_TEST(hex)
{
	static char const str[] = "0123456789012345678901234567890123456789";
	char bin[20];
	TEST_CHECK(aux::from_hex({str, 40}, bin));
	char hex[41];
	aux::to_hex(bin, hex);
	TEST_CHECK(std::strcmp(hex, str) == 0);

	TEST_CHECK(aux::to_hex({"\x55\x73",2}) == "5573");
	TEST_CHECK(aux::to_hex({"\xaB\xd0",2}) == "abd0");

	static char const hex_chars[] = "0123456789abcdefABCDEF";

	for (int i = 1; i < 255; ++i)
	{
		bool const hex_loop = std::strchr(hex_chars, i) != nullptr;
		char const c = char(i);
		TEST_EQUAL(aux::is_hex(c), hex_loop);
	}

	TEST_EQUAL(aux::hex_to_int('0'), 0);
	TEST_EQUAL(aux::hex_to_int('7'), 7);
	TEST_EQUAL(aux::hex_to_int('a'), 10);
	TEST_EQUAL(aux::hex_to_int('f'), 15);
	TEST_EQUAL(aux::hex_to_int('b'), 11);
	TEST_EQUAL(aux::hex_to_int('t'), -1);
	TEST_EQUAL(aux::hex_to_int('g'), -1);
}

TORRENT_TEST(is_space)
{
	TEST_CHECK(!is_space('C'));
	TEST_CHECK(!is_space('\b'));
	TEST_CHECK(!is_space('8'));
	TEST_CHECK(!is_space('='));
	TEST_CHECK(is_space(' '));
	TEST_CHECK(is_space('\t'));
	TEST_CHECK(is_space('\n'));
	TEST_CHECK(is_space('\r'));
	TEST_CHECK(is_space('\f'));
	TEST_CHECK(is_space('\v'));
}

TORRENT_TEST(to_lower)
{
	TEST_CHECK(to_lower('C') == 'c');
	TEST_CHECK(to_lower('c') == 'c');
	TEST_CHECK(to_lower('-') == '-');
	TEST_CHECK(to_lower('&') == '&');
}

TORRENT_TEST(string_equal_no_case)
{
	TEST_CHECK(string_equal_no_case("foobar", "FoobAR"));
	TEST_CHECK(string_equal_no_case("foobar", "foobar"));
	TEST_CHECK(!string_equal_no_case("foobar", "foobar "));
	TEST_CHECK(!string_equal_no_case("foobar", "F00"));
	TEST_CHECK(!string_equal_no_case("foobar", "foo"));
	TEST_CHECK(!string_equal_no_case("foo", "foobar"));

	TEST_CHECK(string_begins_no_case("foobar", "FoobAR --"));
	TEST_CHECK(string_begins_no_case("foo", "foobar"));
	TEST_CHECK(!string_begins_no_case("foobar", "F00"));
	TEST_CHECK(!string_begins_no_case("foobar", "foo"));

	TEST_CHECK(string_ends_with("foobar", "bar"));
	TEST_CHECK(string_ends_with("name.txt", ".txt"));
	TEST_CHECK(string_ends_with("name.a.b", ".a.b"));
	TEST_CHECK(!string_ends_with("-- FoobAR", "foobar"));
	TEST_CHECK(!string_ends_with("foobar", "F00"));
	TEST_CHECK(!string_ends_with("foobar", "foo"));
	TEST_CHECK(!string_ends_with("foo", "foobar"));
}

TORRENT_TEST(to_string)
{
	TEST_CHECK(to_string(345).data() == std::string("345"));
	TEST_CHECK(to_string(-345).data() == std::string("-345"));
	TEST_CHECK(to_string(std::numeric_limits<std::int64_t>::max()).data() == std::string("9223372036854775807"));
	TEST_CHECK(to_string(std::numeric_limits<std::int64_t>::min()).data() == std::string("-9223372036854775808"));

	TEST_CHECK(to_string(0).data() == std::string("0"));
	TEST_CHECK(to_string(10).data() == std::string("10"));
	TEST_CHECK(to_string(100).data() == std::string("100"));
	TEST_CHECK(to_string(1000).data() == std::string("1000"));
	TEST_CHECK(to_string(10000).data() == std::string("10000"));
	TEST_CHECK(to_string(100000).data() == std::string("100000"));
	TEST_CHECK(to_string(1000000).data() == std::string("1000000"));
	TEST_CHECK(to_string(10000000).data() == std::string("10000000"));
	TEST_CHECK(to_string(100000000).data() == std::string("100000000"));
	TEST_CHECK(to_string(1000000000).data() == std::string("1000000000"));
	TEST_CHECK(to_string(10000000000).data() == std::string("10000000000"));
	TEST_CHECK(to_string(100000000000).data() == std::string("100000000000"));
	TEST_CHECK(to_string(1000000000000).data() == std::string("1000000000000"));
	TEST_CHECK(to_string(10000000000000).data() == std::string("10000000000000"));
	TEST_CHECK(to_string(100000000000000).data() == std::string("100000000000000"));
	TEST_CHECK(to_string(1000000000000000).data() == std::string("1000000000000000"));

	TEST_CHECK(to_string(9).data() == std::string("9"));
	TEST_CHECK(to_string(99).data() == std::string("99"));
	TEST_CHECK(to_string(999).data() == std::string("999"));
	TEST_CHECK(to_string(9999).data() == std::string("9999"));
	TEST_CHECK(to_string(99999).data() == std::string("99999"));
	TEST_CHECK(to_string(999999).data() == std::string("999999"));
	TEST_CHECK(to_string(9999999).data() == std::string("9999999"));
	TEST_CHECK(to_string(99999999).data() == std::string("99999999"));
	TEST_CHECK(to_string(999999999).data() == std::string("999999999"));
	TEST_CHECK(to_string(9999999999).data() == std::string("9999999999"));
	TEST_CHECK(to_string(99999999999).data() == std::string("99999999999"));
	TEST_CHECK(to_string(999999999999).data() == std::string("999999999999"));
	TEST_CHECK(to_string(9999999999999).data() == std::string("9999999999999"));
	TEST_CHECK(to_string(99999999999999).data() == std::string("99999999999999"));
	TEST_CHECK(to_string(999999999999999).data() == std::string("999999999999999"));
	TEST_CHECK(to_string(9999999999999999).data() == std::string("9999999999999999"));
	TEST_CHECK(to_string(99999999999999999).data() == std::string("99999999999999999"));
	TEST_CHECK(to_string(999999999999999999).data() == std::string("999999999999999999"));

	TEST_CHECK(to_string(-10).data() == std::string("-10"));
	TEST_CHECK(to_string(-100).data() == std::string("-100"));
	TEST_CHECK(to_string(-1000).data() == std::string("-1000"));
	TEST_CHECK(to_string(-10000).data() == std::string("-10000"));
	TEST_CHECK(to_string(-100000).data() == std::string("-100000"));
	TEST_CHECK(to_string(-1000000).data() == std::string("-1000000"));
	TEST_CHECK(to_string(-10000000).data() == std::string("-10000000"));
	TEST_CHECK(to_string(-100000000).data() == std::string("-100000000"));
	TEST_CHECK(to_string(-1000000000).data() == std::string("-1000000000"));
	TEST_CHECK(to_string(-10000000000).data() == std::string("-10000000000"));
	TEST_CHECK(to_string(-100000000000).data() == std::string("-100000000000"));
	TEST_CHECK(to_string(-1000000000000).data() == std::string("-1000000000000"));
	TEST_CHECK(to_string(-10000000000000).data() == std::string("-10000000000000"));
	TEST_CHECK(to_string(-100000000000000).data() == std::string("-100000000000000"));
	TEST_CHECK(to_string(-1000000000000000).data() == std::string("-1000000000000000"));

	TEST_CHECK(to_string(-9).data() == std::string("-9"));
	TEST_CHECK(to_string(-99).data() == std::string("-99"));
	TEST_CHECK(to_string(-999).data() == std::string("-999"));
	TEST_CHECK(to_string(-9999).data() == std::string("-9999"));
	TEST_CHECK(to_string(-99999).data() == std::string("-99999"));
	TEST_CHECK(to_string(-999999).data() == std::string("-999999"));
	TEST_CHECK(to_string(-9999999).data() == std::string("-9999999"));
	TEST_CHECK(to_string(-99999999).data() == std::string("-99999999"));
	TEST_CHECK(to_string(-999999999).data() == std::string("-999999999"));
	TEST_CHECK(to_string(-9999999999).data() == std::string("-9999999999"));
	TEST_CHECK(to_string(-99999999999).data() == std::string("-99999999999"));
	TEST_CHECK(to_string(-999999999999).data() == std::string("-999999999999"));
	TEST_CHECK(to_string(-9999999999999).data() == std::string("-9999999999999"));
	TEST_CHECK(to_string(-99999999999999).data() == std::string("-99999999999999"));
	TEST_CHECK(to_string(-999999999999999).data() == std::string("-999999999999999"));
	TEST_CHECK(to_string(-9999999999999999).data() == std::string("-9999999999999999"));
	TEST_CHECK(to_string(-99999999999999999).data() == std::string("-99999999999999999"));
	TEST_CHECK(to_string(-999999999999999999).data() == std::string("-999999999999999999"));
}

#if TORRENT_USE_I2P
namespace {

template <size_t N>
std::vector<char> to_vec(char const (&str)[N])
{
	return std::vector<char>(&str[0], &str[N - 1]);
}

std::string to_str(std::vector<char> const& v)
{
	return std::string(v.begin(), v.end());
}

// convert the standard base64 alphabet to the i2p alphabet
std::string transcode_alphabet(std::string in)
{
	std::string ret;
	std::transform(in.begin(), in.end(), std::back_inserter(ret), [](char const c) {
		if (c == '+') return '-';
		if (c == '/') return '~';
		return c;
	});
	return ret;
}
}
#endif

TORRENT_TEST(base64)
{
	// base64 test vectors from http://www.faqs.org/rfcs/rfc4648.html
	TEST_CHECK(base64encode("") == "");
	TEST_CHECK(base64encode("f") == "Zg==");
	TEST_CHECK(base64encode("fo") == "Zm8=");
	TEST_CHECK(base64encode("foo") == "Zm9v");
	TEST_CHECK(base64encode("foob") == "Zm9vYg==");
	TEST_CHECK(base64encode("fooba") == "Zm9vYmE=");
	TEST_CHECK(base64encode("foobar") == "Zm9vYmFy");
#if TORRENT_USE_I2P
	TEST_CHECK(base64decode_i2p("") == to_vec(""));
	TEST_CHECK(base64decode_i2p("Zg==") == to_vec("f"));
	TEST_CHECK(base64decode_i2p("Zm8=") == to_vec("fo"));
	TEST_CHECK(base64decode_i2p("Zm9v") == to_vec("foo"));
	TEST_CHECK(base64decode_i2p("Zm9vYg==") == to_vec("foob"));
	TEST_CHECK(base64decode_i2p("Zm9vYmE=") == to_vec("fooba"));
	TEST_CHECK(base64decode_i2p("Zm9vYmFy") == to_vec("foobar"));

	std::vector<char> test;
	for (int i = 0; i < 255; ++i)
		test.push_back(char(i));
	TEST_CHECK(base64decode_i2p(transcode_alphabet(base64encode(to_str(test)))) == test);
#endif
}

TORRENT_TEST(base32)
{
	// base32 test vectors from http://www.faqs.org/rfcs/rfc4648.html

#if TORRENT_USE_I2P
	// i2p uses lower case and no padding
	TEST_CHECK(base32encode_i2p(""_sv) == "");
	TEST_CHECK(base32encode_i2p("f"_sv) == "my");
	TEST_CHECK(base32encode_i2p("fo"_sv) == "mzxq");
	TEST_CHECK(base32encode_i2p("foo"_sv) == "mzxw6");
	TEST_CHECK(base32encode_i2p("foob"_sv) == "mzxw6yq");
	TEST_CHECK(base32encode_i2p("fooba"_sv) == "mzxw6ytb");
	TEST_CHECK(base32encode_i2p("foobar"_sv) == "mzxw6ytboi");

	std::string test;
	for (int i = 0; i < 255; ++i)
		test += char(i);

	TEST_CHECK(base32decode(base32encode_i2p(test)) == test);
#endif // TORRENT_USE_I2P

	TEST_CHECK(base32decode("") == "");
	TEST_CHECK(base32decode("MY======") == "f");
	TEST_CHECK(base32decode("MZXQ====") == "fo");
	TEST_CHECK(base32decode("MZXW6===") == "foo");
	TEST_CHECK(base32decode("MZXW6YQ=") == "foob");
	TEST_CHECK(base32decode("MZXW6YTB") == "fooba");
	TEST_CHECK(base32decode("MZXW6YTBOI======") == "foobar");

	TEST_CHECK(base32decode("MY") == "f");
	TEST_CHECK(base32decode("MZXW6YQ") == "foob");
	TEST_CHECK(base32decode("MZXW6YTBOI") == "foobar");
	TEST_CHECK(base32decode("mZXw6yTBO1======") == "foobar");

	// make sure invalid encoding returns the empty string
	TEST_CHECK(base32decode("mZXw6yTBO1{#&*()=") == "");
}

TORRENT_TEST(escape_string)
{
	// escape_string
	char const* test_string = "!@#$%^&*()-_=+/,. %?";
	TEST_EQUAL(escape_string(test_string)
		, "!%40%23%24%25%5e%26*()-_%3d%2b%2f%2c.%20%25%3f");

	// escape_path
	TEST_EQUAL(escape_path(test_string)
		, "!%40%23%24%25%5e%26*()-_%3d%2b/%2c.%20%25%3f");

	error_code ec;
	TEST_CHECK(unescape_string(escape_path(test_string), ec) == test_string);
	TEST_CHECK(!ec);
	if (ec) std::printf("%s\n", ec.message().c_str());

	// need_encoding
	char const* test_string2 = "!@$&()-_/,.%?";
	TEST_CHECK(need_encoding(test_string, int(strlen(test_string))) == true);
	TEST_CHECK(need_encoding(test_string2, int(strlen(test_string2))) == false);
	TEST_CHECK(need_encoding("\n", 1) == true);

	// maybe_url_encode
	TEST_EQUAL(maybe_url_encode("http://bla.com/\n"), "http://bla.com/%0a");
	TEST_EQUAL(maybe_url_encode("http://bla.com/foo%20bar"), "http://bla.com/foo%20bar");
	TEST_EQUAL(maybe_url_encode("http://bla.com/foo%20bar?k=v&k2=v2")
		, "http://bla.com/foo%20bar?k=v&k2=v2");
	TEST_EQUAL(maybe_url_encode("?&"), "?&");

	// unescape_string
	TEST_CHECK(unescape_string(escape_string(test_string), ec)
		== test_string);
	std::cout << unescape_string(escape_string(test_string), ec) << std::endl;
	// prematurely terminated string
	unescape_string("%", ec);
	TEST_CHECK(ec == error_code(errors::invalid_escaped_string));
	unescape_string("%0", ec);
	TEST_CHECK(ec == error_code(errors::invalid_escaped_string));

	// invalid hex character
	unescape_string("%GE", ec);
	TEST_CHECK(ec == error_code(errors::invalid_escaped_string));
	unescape_string("%eg", ec);
	TEST_CHECK(ec == error_code(errors::invalid_escaped_string));
	ec.clear();

	TEST_CHECK(unescape_string("123+abc", ec) == "123 abc");
}

TORRENT_TEST(read_until)
{
	char const* test_string1 = "abcdesdf sdgf";
	char const* tmp1 = test_string1;
	TEST_CHECK(read_until(tmp1, 'd', test_string1 + strlen(test_string1)) == "abc");

	tmp1 = test_string1;
	TEST_CHECK(read_until(tmp1, '[', test_string1 + strlen(test_string1))
		== "abcdesdf sdgf");
}

TORRENT_TEST(path)
{
	std::string path = "a\\b\\c";
	convert_path_to_posix(path);
	TEST_EQUAL(path, "a/b/c");
}

namespace {

void test_parse_interface(char const* input
	, std::vector<listen_interface_t> const expected
	, std::vector<std::string> const expected_e
	, string_view const output)
{
	std::printf("parse interface: %s\n", input);
	std::vector<std::string> err;
	auto const list = parse_listen_interfaces(input, err);
	TEST_EQUAL(list.size(), expected.size());
	TEST_CHECK(list == expected);
	TEST_CHECK(err == expected_e);
#if TORRENT_ABI_VERSION == 1 \
	|| !defined TORRENT_DISABLE_LOGGING
	TEST_EQUAL(print_listen_interfaces(list), output);
	std::cout << "RESULT: " << print_listen_interfaces(list) << '\n';
#endif
	TORRENT_UNUSED(output);
	for (auto const& e : err)
		std::cout << "ERR: \"" << e << "\"\n";
}

} // anonymous namespace

TORRENT_TEST(parse_list)
{
	std::vector<std::string> list;
	parse_comma_separated_string("  a,b, c, d ,e \t,foobar\n\r,[::1]", list);
	TEST_CHECK((list == std::vector<std::string>{"a", "b", "c", "d", "e", "foobar", "[::1]"}));
}

TORRENT_TEST(parse_interface)
{
	test_parse_interface("  a:4,b:35, c : 1000s, d: 351 ,e \t:42,foobar:1337s\n\r,[2001::1]:6881"
		, {{"a", 4, false, false}, {"b", 35, false, false}
		, {"c", 1000, true, false}
		, {"d", 351, false, false}
		, {"e", 42, false, false}
		, {"foobar", 1337, true, false}
		, {"2001::1", 6881, false, false}}
		, {}
		, "a:4,b:35,c:1000s,d:351,e:42,foobar:1337s,[2001::1]:6881");

	// IPv6 address
	test_parse_interface("[2001:ffff::1]:6882s"
		, {{"2001:ffff::1", 6882, true, false}}
		, {}
		, "[2001:ffff::1]:6882s");

	// IPv4 address
	test_parse_interface("127.0.0.1:6882"
		, {{"127.0.0.1", 6882, false, false}}
		, {}
		, "127.0.0.1:6882");

	// maximum padding
	test_parse_interface("  nic\r\n:\t 12\r s "
		, {{"nic", 12, true, false}}
		, {}
		, "nic:12s");

	// negative tests
	test_parse_interface("nic:99999999999999999999999", {}, {"nic:99999999999999999999999"}, "");
	test_parse_interface("nic:  -3", {}, {"nic:  -3"}, "");
	test_parse_interface("nic:  ", {}, {"nic:"}, "");
	test_parse_interface("nic :", {}, {"nic :"}, "");
	test_parse_interface("nic ", {}, {"nic"}, "");
	test_parse_interface("nic s", {}, {"nic s"}, "");

	// parse interface with port 0
	test_parse_interface("127.0.0.1:0", {{"127.0.0.1", 0, false, false}}
		, {}, "127.0.0.1:0");

	// SSL flag
	test_parse_interface("127.0.0.1:1234s", {{"127.0.0.1", 1234, true, false}}
		, {}, "127.0.0.1:1234s");
	// local flag
	test_parse_interface("127.0.0.1:1234l", {{"127.0.0.1", 1234, false, true}}
		, {}, "127.0.0.1:1234l");

	// both
	test_parse_interface("127.0.0.1:1234ls", {{"127.0.0.1", 1234, true, true}}
		, {}, "127.0.0.1:1234sl");

	// IPv6 error
	test_parse_interface("[aaaa::1", {}, {"[aaaa::1"}, "");
	test_parse_interface("[aaaa::1]", {}, {"[aaaa::1]"}, "");
	test_parse_interface("[aaaa::1]:", {}, {"[aaaa::1]:"}, "");
	test_parse_interface("[aaaa::1]:s", {}, {"[aaaa::1]:s"}, "");
	test_parse_interface("[aaaa::1] :6881", {}, {"[aaaa::1] :6881"}, "");
	test_parse_interface("[aaaa::1]:6881", {{"aaaa::1", 6881, false, false}}, {}, "[aaaa::1]:6881");

	// unterminated [
	test_parse_interface("[aaaa::1,foobar:0", {{"foobar", 0, false, false}}, {"[aaaa::1"}, "foobar:0");

	// multiple errors
	test_parse_interface("foo:,bar", {}, {"foo:", "bar"}, "");

	// quoted elements
	test_parse_interface("\"abc,.\",bar", {}, {"abc,.", "bar"}, "");

	// silent error
	test_parse_interface("\"", {}, {"\""}, "");

	// multiple errors and one correct
	test_parse_interface("foo,bar,0.0.0.0:6881", {{"0.0.0.0", 6881, false, false}}, {"foo", "bar"}, "0.0.0.0:6881");
}

TORRENT_TEST(split_string_quotes)
{
	TEST_CHECK(split_string_quotes("a b"_sv, ' ') == std::make_pair("a"_sv, "b"_sv));
	TEST_CHECK(split_string_quotes("\"a b\" c"_sv, ' ') == std::make_pair("\"a b\""_sv, "c"_sv));
	TEST_CHECK(split_string_quotes("\"a b\"foobar c"_sv, ' ') == std::make_pair("\"a b\"foobar"_sv, "c"_sv));
	TEST_CHECK(split_string_quotes("a\nb foobar"_sv, ' ') == std::make_pair("a\nb"_sv, "foobar"_sv));
	TEST_CHECK(split_string_quotes("a b\"foo\"bar"_sv, '"') == std::make_pair("a b"_sv, "foo\"bar"_sv));
	TEST_CHECK(split_string_quotes("a"_sv, ' ') == std::make_pair("a"_sv, ""_sv));
	TEST_CHECK(split_string_quotes("\"a b"_sv, ' ') == std::make_pair("\"a b"_sv, ""_sv));
	TEST_CHECK(split_string_quotes(""_sv, ' ') == std::make_pair(""_sv, ""_sv));
}

TORRENT_TEST(split_string)
{
	TEST_CHECK(split_string("a b"_sv, ' ') == std::make_pair("a"_sv, "b"_sv));
	TEST_CHECK(split_string("\"a b\" c"_sv, ' ') == std::make_pair("\"a"_sv, "b\" c"_sv));
	TEST_CHECK(split_string("\"a b\"foobar c"_sv, ' ') == std::make_pair("\"a"_sv, "b\"foobar c"_sv));
	TEST_CHECK(split_string("a\nb foobar"_sv, ' ') == std::make_pair("a\nb"_sv, "foobar"_sv));
	TEST_CHECK(split_string("a b\"foo\"bar"_sv, '"') == std::make_pair("a b"_sv, "foo\"bar"_sv));
	TEST_CHECK(split_string("a"_sv, ' ') == std::make_pair("a"_sv, ""_sv));
	TEST_CHECK(split_string("\"a b"_sv, ' ') == std::make_pair("\"a"_sv, "b"_sv));
	TEST_CHECK(split_string(""_sv, ' ') == std::make_pair(""_sv, ""_sv));
}

TORRENT_TEST(convert_from_native)
{
	TEST_EQUAL(std::string("foobar"), convert_from_native(convert_to_native("foobar")));
	TEST_EQUAL(std::string("foobar")
		, convert_from_native(convert_to_native("foo"))
		+ convert_from_native(convert_to_native("bar")));

	TEST_EQUAL(convert_to_native("foobar")
		, convert_to_native("foo") + convert_to_native("bar"));
}

TORRENT_TEST(trim)
{
	TEST_EQUAL(trim(""), "");
	TEST_EQUAL(trim("\t "), "");
	TEST_EQUAL(trim(" a"), "a");
	TEST_EQUAL(trim(" a "), "a");
	TEST_EQUAL(trim("\t \na \t\r"), "a");
	TEST_EQUAL(trim(" \t \ta"), "a");
	TEST_EQUAL(trim("a "), "a");
	TEST_EQUAL(trim("a \t"), "a");
	TEST_EQUAL(trim("a \t\n \tb"), "a \t\n \tb");
}

#if TORRENT_USE_I2P
TORRENT_TEST(i2p_url)
{
	TEST_CHECK(is_i2p_url("http://a.i2p/a"));
	TEST_CHECK(!is_i2p_url("http://a.I2P/a"));
	TEST_CHECK(!is_i2p_url("http://c.i3p"));
	TEST_CHECK(!is_i2p_url("http://i2p/foo bar"));
}
#endif

TORRENT_TEST(string_ptr_zero_termination)
{
	char str[] = {'f', 'o', 'o', 'b', 'a', 'r'};
	aux::string_ptr p(string_view(str, sizeof(str)));

	// make sure it's zero-terminated now
	TEST_CHECK(strlen(*p) == 6);
	TEST_CHECK((*p)[6] == '\0');
	TEST_CHECK(*p == string_view("foobar"));
}

TORRENT_TEST(string_ptr_move_construct)
{
	aux::string_ptr p1("test");
	TEST_CHECK(*p1 == string_view("test"));

	aux::string_ptr p2(std::move(p1));

	TEST_CHECK(*p2 == string_view("test"));

	// moved-from state is empty
	TEST_CHECK(*p1 == nullptr);
}

TORRENT_TEST(string_ptr_move_assign)
{
	aux::string_ptr p1("test");
	TEST_CHECK(*p1 == string_view("test"));

	aux::string_ptr p2("foobar");

	p1 = std::move(p2);

	TEST_CHECK(*p1 == string_view("foobar"));

	// moved-from state is empty
	TEST_CHECK(*p2 == nullptr);
}

TORRENT_TEST(strip_string)
{
	TEST_EQUAL(strip_string("   ab"), "ab");
	TEST_EQUAL(strip_string("   ab    "), "ab");
	TEST_EQUAL(strip_string("       "), "");
	TEST_EQUAL(strip_string(""), "");
	TEST_EQUAL(strip_string("a     b"), "a     b");
	TEST_EQUAL(strip_string("   a     b   "), "a     b");
	TEST_EQUAL(strip_string(" \t \t ab\t\t\t"), "ab");
}
