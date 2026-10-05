#include "uri.h"
#include <utility>
#include "DlAbortEx.h"

#include <cppunit/extensions/HelperMacros.h>

namespace aria2 {

namespace uri {

class UriTest : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(UriTest);
  CPPUNIT_TEST(testRewriteLocalUrl);
  CPPUNIT_TEST(testConfigureUrlRewrite);
  CPPUNIT_TEST(testSetUri1);
  CPPUNIT_TEST(testSetUri2);
  CPPUNIT_TEST(testSetUri3);
  CPPUNIT_TEST(testSetUri4);
  CPPUNIT_TEST(testSetUri5);
  CPPUNIT_TEST(testSetUri6);
  CPPUNIT_TEST(testSetUri7);
  CPPUNIT_TEST(testSetUri8);
  CPPUNIT_TEST(testSetUri9);
  CPPUNIT_TEST(testSetUri10);
  CPPUNIT_TEST(testSetUri11);
  CPPUNIT_TEST(testSetUri12);
  CPPUNIT_TEST(testSetUri13);
  CPPUNIT_TEST(testSetUri14);
  CPPUNIT_TEST(testSetUri15);
  CPPUNIT_TEST(testSetUri16);
  CPPUNIT_TEST(testSetUri18);
  CPPUNIT_TEST(testSetUri19);
  CPPUNIT_TEST(testSetUri20);
  CPPUNIT_TEST(testSetUri_username);
  CPPUNIT_TEST(testSetUri_usernamePassword);
  CPPUNIT_TEST(testSetUri_zeroUsername);
  CPPUNIT_TEST(testSetUri_ipv6);
  CPPUNIT_TEST(testInnerLink);
  CPPUNIT_TEST(testConstruct);
  CPPUNIT_TEST(testSwap);
  CPPUNIT_TEST(testJoinUri);
  CPPUNIT_TEST(testJoinPath);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp()
  {
    configureUrlRewrite("81.71.15.87:45244=http://localhost:5244\n"
                        "list.hk.jacobwpeng.com=http://localhost:5244\n");
  }
  void tearDown() { configureUrlRewrite(""); }
  void testRewriteLocalUrl();
  void testConfigureUrlRewrite();
  void testSetUri1();
  void testSetUri2();
  void testSetUri3();
  void testSetUri4();
  void testSetUri5();
  void testSetUri6();
  void testSetUri7();
  void testSetUri8();
  void testSetUri9();
  void testSetUri10();
  void testSetUri11();
  void testSetUri12();
  void testSetUri13();
  void testSetUri14();
  void testSetUri15();
  void testSetUri16();
  void testSetUri18();
  void testSetUri19();
  void testSetUri20();
  void testSetUri_username();
  void testSetUri_usernamePassword();
  void testSetUri_zeroUsername();
  void testSetUri_ipv6();
  void testInnerLink();
  void testConstruct();
  void testSwap();
  void testJoinUri();
  void testJoinPath();
};

CPPUNIT_TEST_SUITE_REGISTRATION(UriTest);

void UriTest::testRewriteLocalUrl()
{
  const std::pair<std::string, std::string> cases[] = {
      {"http://81.71.15.87:45244/d/a%2Fb?sign=A%2B+B&x=1#frag",
       "http://localhost:5244/d/a%2Fb?sign=A%2B+B&x=1#frag"},
      {"https://LIST.HK.JACOBWPENG.COM:443/d/中文 空格",
       "http://localhost:5244/d/中文 空格"},
      {"ftp://user:pass@list.hk.jacobwpeng.com:123/file",
       "http://localhost:5244/file"},
      {"custom+scheme://list.hk.jacobwpeng.com?x=%00",
       "http://localhost:5244?x=%00"},
      {"http://list.hk.jacobwpeng.com", "http://localhost:5244"},
      {"http://81.71.15.87:45244", "http://localhost:5244"},
      {"h\tttp://list.hk.jacobwpeng.com/a\r\nb\x7f?x=1",
       "http://localhost:5244/ab?x=1"},
      {std::string("http://list.hk.jacobwpeng.com/a\0b", 33),
       "http://localhost:5244/ab"},
      {"http://81.71.15.87:45245/file", "http://81.71.15.87:45245/file"},
      {"http://81.71.15.87:045244/file", "http://81.71.15.87:045244/file"},
      {"http://81.71.15.87/file", "http://81.71.15.87/file"},
      {"http://list.hk.jacobwpeng.com.evil/file",
       "http://list.hk.jacobwpeng.com.evil/file"},
      {"http://list.hk.jacobwpeng.com@evil/file",
       "http://list.hk.jacobwpeng.com@evil/file"},
      {"http://evil/file?url=http://list.hk.jacobwpeng.com/file",
       "http://evil/file?url=http://list.hk.jacobwpeng.com/file"},
      {"http://[::1]:5244/file", "http://[::1]:5244/file"},
      {"http://localhost:5244/file?x=%2F", "http://localhost:5244/file?x=%2F"},
      {"http://example.com/a\tb", "http://example.com/a\tb"},
      {"magnet:?xt=urn:btih:123", "magnet:?xt=urn:btih:123"},
      {"/tmp/list.hk.jacobwpeng.com/file", "/tmp/list.hk.jacobwpeng.com/file"},
      {"1http://list.hk.jacobwpeng.com/file",
       "1http://list.hk.jacobwpeng.com/file"},
      {"http://", "http://"}, {"", ""}};
  for (const auto& entry : cases) {
    auto rewritten = rewriteLocalUrl(entry.first);
    CPPUNIT_ASSERT_EQUAL(entry.second, rewritten);
    CPPUNIT_ASSERT_EQUAL(rewritten, rewriteLocalUrl(rewritten));
  }
}

void UriTest::testConfigureUrlRewrite()
{
  configureUrlRewrite("");
  CPPUNIT_ASSERT_EQUAL(std::string("http://list.hk.jacobwpeng.com/file"),
                       rewriteLocalUrl("http://list.hk.jacobwpeng.com/file"));
  configureUrlRewrite(" # comment\n SOURCE.TEST:81 = http://localhost:5255/ \r\n"
                      "source.test=https://mirror.invalid:7443\n"
                      "[2001:db8::1]:123=http://localhost:5233\n");
  CPPUNIT_ASSERT_EQUAL(std::string("http://localhost:5255/file?x=%2F#frag"),
                       rewriteLocalUrl("https://SOURCE.TEST:81/file?x=%2F#frag"));
  const std::string expected = "https://mirror.invalid:7443/file?x=%2F#frag";
  CPPUNIT_ASSERT_EQUAL(expected, rewriteLocalUrl("http://source.test/file?x=%2F#frag"));
  CPPUNIT_ASSERT_EQUAL(expected, rewriteLocalUrl(expected));
  CPPUNIT_ASSERT_EQUAL(std::string("http://localhost:5233/file"),
                       rewriteLocalUrl("http://[2001:db8::1]:123/file"));
  const char* invalid[] = {
      "missing-separator", "=http://localhost:5244", "source.test=",
      "source.test=ftp://localhost:5244", "source.test=http://localhost:5244/path",
      "source.test=http://localhost:5244?query", "source.test=http://localhost:5244#frag",
      "source.test=http://user:pass@localhost:5244", "source.test=http://localhost:0",
      "source.test=http://localhost:65536", "source.test:0=http://localhost:5244",
      "source.test:65536=http://localhost:5244", "*.test=http://localhost:5244",
      "http://source.test=http://localhost:5244", "source.test=http://source.test",
      "source.test=http://next.test\nnext.test=http://localhost:5244",
      "source.test=http://localhost:5244\nSOURCE.TEST=http://localhost:5233"};
  for (const auto& rule : invalid) {
    CPPUNIT_ASSERT_THROW(configureUrlRewrite(rule), DlAbortEx);
    CPPUNIT_ASSERT_EQUAL(expected, rewriteLocalUrl("http://source.test/file?x=%2F#frag"));
  }
}

void UriTest::testSetUri1()
{
  UriStruct us;
  bool v = parse(us, "http://aria.rednoah.com/");

  CPPUNIT_ASSERT(v);
  CPPUNIT_ASSERT_EQUAL(std::string("http"), us.protocol);
  CPPUNIT_ASSERT_EQUAL((uint16_t)80, us.port);
  CPPUNIT_ASSERT_EQUAL(std::string("aria.rednoah.com"), us.host);
  CPPUNIT_ASSERT_EQUAL(std::string("/"), us.dir);
  CPPUNIT_ASSERT_EQUAL(std::string(""), us.file);
  CPPUNIT_ASSERT_EQUAL(std::string(""), us.query);
  CPPUNIT_ASSERT_EQUAL(std::string(""), us.username);
  CPPUNIT_ASSERT_EQUAL(std::string(""), us.password);
  CPPUNIT_ASSERT(!us.ipv6LiteralAddress);
}

void UriTest::testSetUri2()
{
  UriStruct us;
  bool v = parse(us, "http://aria.rednoah.com:8080/index.html");

  CPPUNIT_ASSERT(v);
  CPPUNIT_ASSERT_EQUAL(std::string("http"), us.protocol);
  CPPUNIT_ASSERT_EQUAL((uint16_t)8080, us.port);
  CPPUNIT_ASSERT_EQUAL(std::string("aria.rednoah.com"), us.host);
  CPPUNIT_ASSERT_EQUAL(std::string("/"), us.dir);
  CPPUNIT_ASSERT_EQUAL(std::string("index.html"), us.file);
  CPPUNIT_ASSERT_EQUAL(std::string(""), us.query);
}

void UriTest::testSetUri3()
{
  UriStruct us;
  bool v = parse(us, "http://aria.rednoah.com/aria2/index.html");

  CPPUNIT_ASSERT(v);
  CPPUNIT_ASSERT_EQUAL(std::string("http"), us.protocol);
  CPPUNIT_ASSERT_EQUAL((uint16_t)80, us.port);
  CPPUNIT_ASSERT_EQUAL(std::string("aria.rednoah.com"), us.host);
  CPPUNIT_ASSERT_EQUAL(std::string("/aria2/"), us.dir);
  CPPUNIT_ASSERT_EQUAL(std::string("index.html"), us.file);
  CPPUNIT_ASSERT_EQUAL(std::string(""), us.query);
}

void UriTest::testSetUri4()
{
  UriStruct us;
  bool v = parse(us, "http://aria.rednoah.com/aria2/aria3/index.html");

  CPPUNIT_ASSERT(v);
  CPPUNIT_ASSERT_EQUAL(std::string("http"), us.protocol);
  CPPUNIT_ASSERT_EQUAL((uint16_t)80, us.port);
  CPPUNIT_ASSERT_EQUAL(std::string("aria.rednoah.com"), us.host);
  CPPUNIT_ASSERT_EQUAL(std::string("/aria2/aria3/"), us.dir);
  CPPUNIT_ASSERT_EQUAL(std::string("index.html"), us.file);
  CPPUNIT_ASSERT_EQUAL(std::string(""), us.query);
}

void UriTest::testSetUri5()
{
  UriStruct us;
  bool v = parse(us, "http://aria.rednoah.com/aria2/aria3/");

  CPPUNIT_ASSERT(v);
  CPPUNIT_ASSERT_EQUAL(std::string("http"), us.protocol);
  CPPUNIT_ASSERT_EQUAL((uint16_t)80, us.port);
  CPPUNIT_ASSERT_EQUAL(std::string("aria.rednoah.com"), us.host);
  CPPUNIT_ASSERT_EQUAL(std::string("/aria2/aria3/"), us.dir);
  CPPUNIT_ASSERT_EQUAL(std::string(""), us.file);
  CPPUNIT_ASSERT_EQUAL(std::string(""), us.query);
}

void UriTest::testSetUri6()
{
  UriStruct us;
  bool v = parse(us, "http://aria.rednoah.com/aria2/aria3");

  CPPUNIT_ASSERT(v);
  CPPUNIT_ASSERT_EQUAL(std::string("http"), us.protocol);
  CPPUNIT_ASSERT_EQUAL((uint16_t)80, us.port);
  CPPUNIT_ASSERT_EQUAL(std::string("aria.rednoah.com"), us.host);
  CPPUNIT_ASSERT_EQUAL(std::string("/aria2/"), us.dir);
  CPPUNIT_ASSERT_EQUAL(std::string("aria3"), us.file);
  CPPUNIT_ASSERT_EQUAL(std::string(""), us.query);
}

void UriTest::testSetUri7()
{
  UriStruct us;
  bool v = parse(us, "http://");

  CPPUNIT_ASSERT(!v);
}

void UriTest::testSetUri8()
{
  UriStruct us;
  bool v = parse(us, "http:/aria.rednoah.com");

  CPPUNIT_ASSERT(!v);
}

void UriTest::testSetUri9()
{
  UriStruct us;
  bool v = parse(us, "h");

  CPPUNIT_ASSERT(!v);
}

void UriTest::testSetUri10()
{
  UriStruct us;
  bool v = parse(us, "");

  CPPUNIT_ASSERT(!v);
}

void UriTest::testSetUri11()
{
  UriStruct us;
  bool v = parse(us, "http://host?query/");

  CPPUNIT_ASSERT(v);
  CPPUNIT_ASSERT_EQUAL(std::string("http"), us.protocol);
  CPPUNIT_ASSERT_EQUAL(std::string("host"), us.host);
  CPPUNIT_ASSERT_EQUAL(std::string("/"), us.dir);
  CPPUNIT_ASSERT_EQUAL(std::string(""), us.file);
  CPPUNIT_ASSERT_EQUAL(std::string("?query/"), us.query);
}

void UriTest::testSetUri12()
{
  UriStruct us;
  bool v = parse(us, "http://host?query");

  CPPUNIT_ASSERT(v);
  CPPUNIT_ASSERT_EQUAL(std::string("http"), us.protocol);
  CPPUNIT_ASSERT_EQUAL(std::string("host"), us.host);
  CPPUNIT_ASSERT_EQUAL(std::string("/"), us.dir);
  CPPUNIT_ASSERT_EQUAL(std::string(""), us.file);
  CPPUNIT_ASSERT_EQUAL(std::string("?query"), us.query);
}

void UriTest::testSetUri13()
{
  UriStruct us;
  bool v = parse(us, "http://host/?query");

  CPPUNIT_ASSERT(v);
  CPPUNIT_ASSERT_EQUAL(std::string("http"), us.protocol);
  CPPUNIT_ASSERT_EQUAL(std::string("host"), us.host);
  CPPUNIT_ASSERT_EQUAL(std::string("/"), us.dir);
  CPPUNIT_ASSERT_EQUAL(std::string(""), us.file);
  CPPUNIT_ASSERT_EQUAL(std::string("?query"), us.query);
}

void UriTest::testSetUri14()
{
  UriStruct us;
  bool v = parse(us, "http://host:8080/abc?query");

  CPPUNIT_ASSERT(v);
  CPPUNIT_ASSERT_EQUAL(std::string("http"), us.protocol);
  CPPUNIT_ASSERT_EQUAL(std::string("host"), us.host);
  CPPUNIT_ASSERT_EQUAL((uint16_t)8080, us.port);
  CPPUNIT_ASSERT_EQUAL(std::string("/"), us.dir);
  CPPUNIT_ASSERT_EQUAL(std::string("abc"), us.file);
  CPPUNIT_ASSERT_EQUAL(std::string("?query"), us.query);
}

void UriTest::testSetUri15()
{
  UriStruct us;
  // 2 slashes after host name and dir
  bool v = parse(us, "http://host//dir1/dir2//file");
  CPPUNIT_ASSERT(v);
  CPPUNIT_ASSERT_EQUAL(std::string("http"), us.protocol);
  CPPUNIT_ASSERT_EQUAL(std::string("host"), us.host);
  CPPUNIT_ASSERT_EQUAL(std::string("//dir1/dir2//"), us.dir);
  CPPUNIT_ASSERT_EQUAL(std::string("file"), us.file);
  CPPUNIT_ASSERT_EQUAL(std::string(""), us.query);
}

void UriTest::testSetUri16()
{
  UriStruct us;
  // 2 slashes before file
  bool v = parse(us, "http://host//file");
  CPPUNIT_ASSERT(v);
  CPPUNIT_ASSERT_EQUAL(std::string("http"), us.protocol);
  CPPUNIT_ASSERT_EQUAL(std::string("host"), us.host);
  CPPUNIT_ASSERT_EQUAL(std::string("//"), us.dir);
  CPPUNIT_ASSERT_EQUAL(std::string("file"), us.file);
  CPPUNIT_ASSERT_EQUAL(std::string(""), us.query);
}

void UriTest::testSetUri18()
{
  UriStruct us;
  bool v = parse(us, "http://1/");

  CPPUNIT_ASSERT(v);
}

void UriTest::testSetUri19()
{
  UriStruct us;
  // No host
  bool v = parse(us, "http://user@");

  CPPUNIT_ASSERT(!v);
}

void UriTest::testSetUri20()
{
  UriStruct us;
  bool v;
  // Invalid port
  v = parse(us, "http://localhost:65536");
  CPPUNIT_ASSERT(!v);
  v = parse(us, "http://localhost:65535");
  CPPUNIT_ASSERT(v);
  v = parse(us, "http://localhost:-80");
  CPPUNIT_ASSERT(!v);
}

void UriTest::testSetUri_zeroUsername()
{
  UriStruct us;
  CPPUNIT_ASSERT(!parse(us, "ftp://@localhost/download/aria2-1.0.0.tar.bz2"));

  CPPUNIT_ASSERT(!parse(us, "ftp://:@localhost/download/aria2-1.0.0.tar.bz2"));

  CPPUNIT_ASSERT(
      !parse(us, "ftp://:pass@localhost/download/aria2-1.0.0.tar.bz2"));
}

void UriTest::testSetUri_username()
{
  UriStruct us;
  CPPUNIT_ASSERT(
      parse(us, "ftp://aria2@user@localhost/download/aria2-1.0.0.tar.bz2"));
  CPPUNIT_ASSERT_EQUAL(std::string("ftp"), us.protocol);
  CPPUNIT_ASSERT_EQUAL((uint16_t)21, us.port);
  CPPUNIT_ASSERT_EQUAL(std::string("localhost"), us.host);
  CPPUNIT_ASSERT_EQUAL(std::string("/download/"), us.dir);
  CPPUNIT_ASSERT_EQUAL(std::string("aria2-1.0.0.tar.bz2"), us.file);
  CPPUNIT_ASSERT_EQUAL(std::string("aria2@user"), us.username);
  CPPUNIT_ASSERT_EQUAL(std::string(""), us.password);
}

void UriTest::testSetUri_usernamePassword()
{
  UriStruct us;
  CPPUNIT_ASSERT(parse(us,
                       "ftp://aria2@user%40:aria2@pass%40@localhost/download/"
                       "aria2-1.0.0.tar.bz2"));
  CPPUNIT_ASSERT_EQUAL(std::string("ftp"), us.protocol);
  CPPUNIT_ASSERT_EQUAL((uint16_t)21, us.port);
  CPPUNIT_ASSERT_EQUAL(std::string("pass%40@localhost"), us.host);
  CPPUNIT_ASSERT_EQUAL(std::string("/download/"), us.dir);
  CPPUNIT_ASSERT_EQUAL(std::string("aria2-1.0.0.tar.bz2"), us.file);
  CPPUNIT_ASSERT_EQUAL(std::string("aria2@user@"), us.username);
  CPPUNIT_ASSERT_EQUAL(std::string("aria2"), us.password);

  // make sure that after new uri is set, username and password are updated.
  CPPUNIT_ASSERT(parse(us, "ftp://localhost/download/aria2-1.0.0.tar.bz2"));
  CPPUNIT_ASSERT_EQUAL(std::string(""), us.username);
  CPPUNIT_ASSERT_EQUAL(std::string(""), us.password);
}

void UriTest::testSetUri_ipv6()
{
  UriStruct us;
  CPPUNIT_ASSERT(!parse(us, "http://[::1"));
  CPPUNIT_ASSERT(parse(us, "http://[::1]"));
  CPPUNIT_ASSERT_EQUAL(std::string("::1"), us.host);

  CPPUNIT_ASSERT(parse(us, "http://[::1]:8000/dir/file"));
  CPPUNIT_ASSERT_EQUAL(std::string("::1"), us.host);
  CPPUNIT_ASSERT_EQUAL((uint16_t)8000, us.port);
  CPPUNIT_ASSERT_EQUAL(std::string("/dir/"), us.dir);
  CPPUNIT_ASSERT_EQUAL(std::string("file"), us.file);
  CPPUNIT_ASSERT(us.ipv6LiteralAddress);
}

void UriTest::testInnerLink()
{
  UriStruct us;
  bool v = parse(us, "http://aria.rednoah.com/index.html#download");
  CPPUNIT_ASSERT(v);
  CPPUNIT_ASSERT_EQUAL(std::string("index.html"), us.file);
  CPPUNIT_ASSERT_EQUAL(std::string(""), us.query);
}

void UriTest::testConstruct()
{
  {
    UriStruct us;
    CPPUNIT_ASSERT(parse(us, "http://host/dir/file?q=abc#foo"));
    CPPUNIT_ASSERT_EQUAL(std::string("http://host/dir/file?q=abc"),
                         construct(us));
  }
  {
    UriStruct us;
    CPPUNIT_ASSERT(parse(us, "http://host/dir/file"));
    CPPUNIT_ASSERT_EQUAL(std::string("http://host/dir/file"), construct(us));
  }
  {
    UriStruct us;
    CPPUNIT_ASSERT(parse(us, "http://host/dir/"));
    CPPUNIT_ASSERT_EQUAL(std::string("http://host/dir/"), construct(us));
  }
  {
    UriStruct us;
    CPPUNIT_ASSERT(parse(us, "http://host/dir"));
    CPPUNIT_ASSERT_EQUAL(std::string("http://host/dir"), construct(us));
  }
  {
    UriStruct us;
    CPPUNIT_ASSERT(parse(us, "http://host/"));
    CPPUNIT_ASSERT_EQUAL(std::string("http://host/"), construct(us));
  }
  {
    UriStruct us;
    CPPUNIT_ASSERT(parse(us, "http://host"));
    CPPUNIT_ASSERT_EQUAL(std::string("http://host/"), construct(us));
  }
  {
    UriStruct us;
    us.protocol = "http";
    us.host = "host";
    us.file = "foo.xml";
    CPPUNIT_ASSERT_EQUAL(std::string("http://host/foo.xml"), construct(us));
  }
  {
    UriStruct us;
    CPPUNIT_ASSERT(parse(us, "http://host:80"));
    CPPUNIT_ASSERT_EQUAL(std::string("http://host/"), construct(us));
  }
  {
    UriStruct us;
    CPPUNIT_ASSERT(parse(us, "http://host:8080"));
    CPPUNIT_ASSERT_EQUAL(std::string("http://host:8080/"), construct(us));
  }
  {
    UriStruct us;
    CPPUNIT_ASSERT(parse(us, "http://[::1]:8000/dir/file"));
    CPPUNIT_ASSERT_EQUAL(std::string("http://[::1]:8000/dir/file"),
                         construct(us));
  }
  {
    UriStruct us;
    CPPUNIT_ASSERT(parse(us, "ftp://user%40@host/dir/file"));
    CPPUNIT_ASSERT_EQUAL(std::string("ftp://user%40@host/dir/file"),
                         construct(us));
  }
  {
    UriStruct us;
    CPPUNIT_ASSERT(parse(us, "ftp://user:@host/dir/file"));
    CPPUNIT_ASSERT_EQUAL(std::string("ftp://user:@host/dir/file"),
                         construct(us));
  }
  {
    UriStruct us;
    CPPUNIT_ASSERT(parse(us, "ftp://user:passwd%40@host/dir/file"));
    CPPUNIT_ASSERT_EQUAL(std::string("ftp://user:passwd%40@host/dir/file"),
                         construct(us));
  }
}

void UriTest::testSwap()
{
  UriStruct us1;
  CPPUNIT_ASSERT(parse(us1, "http://u1:p1@[::1]/dir1/file1?k1=v1"));
  UriStruct us2;
  CPPUNIT_ASSERT(parse(us2, "ftp://host2/dir2/file2?k2=v2"));
  us1.swap(us2);
  CPPUNIT_ASSERT_EQUAL(std::string("ftp://host2/dir2/file2?k2=v2"),
                       construct(us1));
  CPPUNIT_ASSERT_EQUAL(std::string("http://u1:p1@[::1]/dir1/file1?k1=v1"),
                       construct(us2));
}

void UriTest::testJoinUri()
{
  CPPUNIT_ASSERT_EQUAL(std::string("http://host/dir/file"),
                       joinUri("http://base/d/f", "http://host/dir/file"));

  CPPUNIT_ASSERT_EQUAL(std::string("http://base/dir/file"),
                       joinUri("http://base/d/f", "/dir/file"));

  CPPUNIT_ASSERT_EQUAL(std::string("http://base/d/dir/file"),
                       joinUri("http://base/d/f", "dir/file"));

  CPPUNIT_ASSERT_EQUAL(std::string("http://base/d/"),
                       joinUri("http://base/d/f", ""));

  CPPUNIT_ASSERT_EQUAL(std::string("http://base/d/dir/file?q=k"),
                       joinUri("http://base/d/f", "dir/file?q=k"));

  CPPUNIT_ASSERT_EQUAL(std::string("dir/file"), joinUri("baduri", "dir/file"));

  CPPUNIT_ASSERT_EQUAL(std::string("http://base/a/b/d/file"),
                       joinUri("http://base/a/b/c/x", "../d/file"));

  CPPUNIT_ASSERT_EQUAL(std::string("http://base/a/b/file"),
                       joinUri("http://base/c/x", "../../a/b/file"));

  CPPUNIT_ASSERT_EQUAL(std::string("http://base/"),
                       joinUri("http://base/c/x", "../.."));

  CPPUNIT_ASSERT_EQUAL(std::string("http://base/"),
                       joinUri("http://base/c/x", ".."));

  CPPUNIT_ASSERT_EQUAL(std::string("http://base/a/file"),
                       joinUri("http://base/b/c/x", "/a/x/../file"));

  CPPUNIT_ASSERT_EQUAL(std::string("http://base/file"),
                       joinUri("http://base/f/?q=k", "/file"));

  CPPUNIT_ASSERT_EQUAL(std::string("http://base/file?q=/"),
                       joinUri("http://base/", "/file?q=/"));

  CPPUNIT_ASSERT_EQUAL(std::string("http://base/file?q=v"),
                       joinUri("http://base/", "/file?q=v#a?q=x"));

  CPPUNIT_ASSERT_EQUAL(std::string("http://base/file"),
                       joinUri("http://base/", "/file#a?q=x"));
}

void UriTest::testJoinPath()
{
  CPPUNIT_ASSERT_EQUAL(std::string("/b"), joinPath("/a", "/b"));
  CPPUNIT_ASSERT_EQUAL(std::string("/alpha/bravo"),
                       joinPath("/alpha", "bravo"));
  CPPUNIT_ASSERT_EQUAL(std::string("/bravo"),
                       joinPath("/a", "/alpha/../bravo"));
  CPPUNIT_ASSERT_EQUAL(std::string("/alpha/charlie/"),
                       joinPath("/a", "/alpha/bravo/../charlie/"));
  CPPUNIT_ASSERT_EQUAL(std::string("/alpha/bravo/"),
                       joinPath("/a", "/alpha////bravo//"));
  CPPUNIT_ASSERT_EQUAL(std::string("/alpha/bravo/"),
                       joinPath("/a", "/alpha/././bravo/"));
  CPPUNIT_ASSERT_EQUAL(std::string("/alpha/bravo/"),
                       joinPath("/a", "/alpha/bravo/./"));
  CPPUNIT_ASSERT_EQUAL(std::string("/alpha/bravo/"),
                       joinPath("/a", "/alpha/bravo/."));
  CPPUNIT_ASSERT_EQUAL(std::string("/alpha/"),
                       joinPath("/a", "/alpha/bravo/.."));
  CPPUNIT_ASSERT_EQUAL(std::string("/alpha/"), joinPath("/", "../alpha/"));
  CPPUNIT_ASSERT_EQUAL(std::string("/bravo/"), joinPath("/alpha", "../bravo/"));
  CPPUNIT_ASSERT_EQUAL(std::string("/bravo/"),
                       joinPath("/alpha", "../../bravo/"));
  // If neither paths do not start with '/', the resulting path also
  // does not start with '/'.
  CPPUNIT_ASSERT_EQUAL(std::string("alpha/bravo"), joinPath("alpha", "bravo"));
  CPPUNIT_ASSERT_EQUAL(std::string("bravo/"),
                       joinPath("alpha", "../../bravo/"));
}

} // namespace uri

} // namespace aria2
