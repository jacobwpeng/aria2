/* <!-- copyright */
/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2010 Tatsuhiro Tsujikawa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
/* copyright --> */
#include "uri.h"
#include <algorithm>
#include <sstream>
#include <vector>
#include "DlAbortEx.h"
#include "A2STR.h"
#include "FeatureConfig.h"
#include "util.h"

namespace aria2 {

namespace uri {

UriStruct::UriStruct() : port(0), hasPassword(false), ipv6LiteralAddress(false)
{
}

UriStruct::UriStruct(const UriStruct& c) = default;

UriStruct::~UriStruct() = default;

UriStruct& UriStruct::operator=(const UriStruct& c)
{
  if (this != &c) {
    protocol = c.protocol;
    host = c.host;
    dir = c.dir;
    file = c.file;
    query = c.query;
    username = c.username;
    password = c.password;
    port = c.port;
    hasPassword = c.hasPassword;
    ipv6LiteralAddress = c.ipv6LiteralAddress;
  }
  return *this;
}

void UriStruct::swap(UriStruct& other)
{
  using std::swap;
  if (this != &other) {
    swap(protocol, other.protocol);
    swap(host, other.host);
    swap(dir, other.dir);
    swap(file, other.file);
    swap(query, other.query);
    swap(username, other.username);
    swap(password, other.password);
    swap(port, other.port);
    swap(hasPassword, other.hasPassword);
    swap(ipv6LiteralAddress, other.ipv6LiteralAddress);
  }
}

void swap(UriStruct& lhs, UriStruct& rhs) { lhs.swap(rhs); }

namespace {
struct UrlRewriteRule {
  std::string host;
  std::string port;
  std::string target;
};

std::vector<UrlRewriteRule> urlRewriteRules;

std::string explicitPort(const uri_split_result& parts, const std::string& uri)
{
  if (!(parts.field_set & (1 << USR_PORT))) {
    return "";
  }
  // uri_split sets the numeric port and its presence bit, but does not
  // populate fields[USR_PORT]. Keep the original spelling for exact matching.
  auto end = uri.find_first_of("/?#", uri.find("://") + 3);
  if (end == std::string::npos) {
    end = uri.size();
  }
  auto colon = uri.rfind(':', end - 1);
  return uri.substr(colon + 1, end - colon - 1);
}
} // namespace

void configureUrlRewrite(const std::string& rules)
{
  std::vector<UrlRewriteRule> parsed;
  std::istringstream input(rules);
  std::string line;
  size_t lineNumber = 0;
  while (std::getline(input, line)) {
    ++lineNumber;
    line = util::strip(line);
    if (line.empty() || line[0] == '#') {
      continue;
    }
    auto eq = line.find('=');
    auto source = util::strip(line.substr(0, eq));
    auto target = eq == std::string::npos ? std::string()
                                        : util::strip(line.substr(eq + 1));
    auto sourceUri = "http://" + source;
    uri_split_result sourceParts{}, targetParts{};
    UriStruct destination;
    if (source.empty() || source.find_first_of("@/?#\\*") != std::string::npos ||
        uri_split(&sourceParts, sourceUri.c_str()) != 0 ||
        ((sourceParts.field_set & (1 << USR_PORT)) && sourceParts.port == 0) ||
        target.find_first_of("@?#") != std::string::npos ||
        std::any_of(line.begin(), line.end(),
                    [](unsigned char c) { return c < 0x20 || c == 0x7f; }) ||
        uri_split(&targetParts, target.c_str()) != 0 ||
        ((targetParts.field_set & (1 << USR_PORT)) && targetParts.port == 0) ||
        !parse(destination, target) ||
        (destination.protocol != "http" && destination.protocol != "https") ||
        destination.dir != "/" || !destination.file.empty()) {
      throw DL_ABORT_EX(fmt("Invalid url-rewrite rule #%zu; expected "
                           "HOST[:PORT]=HTTP(S)_ORIGIN", lineNumber));
    }
    UrlRewriteRule rule{
        util::toLower(getFieldString(sourceParts, USR_HOST, sourceUri.c_str())),
        explicitPort(sourceParts, sourceUri), target};
    if (!rule.target.empty() && rule.target.back() == '/') {
      rule.target.pop_back();
    }
    for (const auto& previous : parsed) {
      if (rule.host == previous.host && rule.port == previous.port) {
        throw DL_ABORT_EX(fmt("Duplicate url-rewrite source in rule #%zu",
                             lineNumber));
      }
    }
    parsed.push_back(std::move(rule));
  }
  // The same URL passes through several core entry points. Reject targets
  // that would match a source, so repeated rewrites remain idempotent.
  for (const auto& rule : parsed) {
    uri_split_result parts{};
    uri_split(&parts, rule.target.c_str());
    auto host = util::toLower(getFieldString(parts, USR_HOST, rule.target.c_str()));
    auto port = explicitPort(parts, rule.target);
    for (const auto& source : parsed) {
      if (host == source.host && (source.port.empty() || port == source.port)) {
        throw DL_ABORT_EX("A url-rewrite target matches a configured source; "
                          "chained rewrites are not supported");
      }
    }
  }
  urlRewriteRules.swap(parsed);
}

std::string rewriteLocalUrl(const std::string& uri)
{
  if (urlRewriteRules.empty()) {
    return uri;
  }
  // Match the former AriaNg decorator: strip ASCII controls only when
  // the normalized authority matches one of the configured sources.
  auto normalized = uri;
  normalized.erase(
      std::remove_if(normalized.begin(), normalized.end(),
                     [](unsigned char c) { return c < 0x20 || c == 0x7f; }),
      normalized.end());
  auto schemeEnd = normalized.find("://");
  if (schemeEnd == std::string::npos || schemeEnd == 0 ||
      !util::isAlpha(normalized[0])) {
    return uri;
  }
  for (size_t i = 1; i < schemeEnd; ++i) {
    auto c = normalized[i];
    if (!util::isAlpha(c) && !util::isDigit(c) && c != '+' && c != '-' &&
        c != '.') {
      return uri;
    }
  }
  auto authorityStart = schemeEnd + 3;
  auto authorityEnd = normalized.find_first_of("/?#", authorityStart);
  if (authorityEnd == std::string::npos) {
    authorityEnd = normalized.size();
  }
  auto hostport = normalized.substr(authorityStart,
                                     authorityEnd - authorityStart);
  auto at = hostport.rfind('@');
  if (at != std::string::npos) {
    hostport.erase(0, at + 1);
  }
  std::string host, port;
  if (!hostport.empty() && hostport[0] == '[') {
    auto end = hostport.find(']');
    if (end == std::string::npos) {
      return uri;
    }
    host = hostport.substr(1, end - 1);
    if (end + 1 < hostport.size() && hostport[end + 1] == ':') {
      port = hostport.substr(end + 2);
    }
  }
  else {
    auto colon = hostport.rfind(':');
    if (colon != std::string::npos && colon + 1 < hostport.size() &&
        std::all_of(hostport.begin() + colon + 1, hostport.end(),
                    [](char c) { return c >= '0' && c <= '9'; })) {
      host = hostport.substr(0, colon);
      port = hostport.substr(colon + 1);
    }
    else {
      host = hostport;
    }
  }
  host = util::toLower(host);
  for (const auto& rule : urlRewriteRules) {
    if (host == rule.host && (rule.port.empty() || port == rule.port)) {
      return rule.target + normalized.substr(authorityEnd);
    }
  }
  return uri;
}

bool parse(UriStruct& result, const std::string& uri)
{
  uri_split_result res;
  int rv;
  const char* p = uri.c_str();
  rv = uri_split(&res, p);
  if (rv != 0) {
    return false;
  }

  result.protocol.assign(p + res.fields[USR_SCHEME].off,
                         res.fields[USR_SCHEME].len);
  result.host.assign(p + res.fields[USR_HOST].off, res.fields[USR_HOST].len);
  if (res.port == 0) {
    uint16_t defPort;
    if ((defPort = getDefaultPort(result.protocol)) == 0) {
      return false;
    }
    result.port = defPort;
  }
  else {
    result.port = res.port;
  }

  if (res.field_set & (1 << USR_PATH)) {
    if (res.field_set & (1 << USR_BASENAME)) {
      result.dir.assign(p + res.fields[USR_PATH].off,
                        res.fields[USR_PATH].len -
                            res.fields[USR_BASENAME].len);
      result.file.assign(p + res.fields[USR_BASENAME].off,
                         res.fields[USR_BASENAME].len);
    }
    else {
      result.dir.assign(p + res.fields[USR_PATH].off, res.fields[USR_PATH].len);
      result.file = A2STR::NIL;
    }
  }
  else {
    result.dir = "/";
    result.file = A2STR::NIL;
  }

  if (res.field_set & (1 << USR_QUERY)) {
    result.query = "?";
    result.query.append(p + res.fields[USR_QUERY].off,
                        res.fields[USR_QUERY].len);
  }
  else {
    result.query = A2STR::NIL;
  }

  if (res.field_set & (1 << USR_USER)) {
    result.username.assign(p + res.fields[USR_USER].off,
                           res.fields[USR_USER].len);
    result.username =
        util::percentDecode(result.username.begin(), result.username.end());
  }
  else {
    result.username = A2STR::NIL;
  }

  if (res.field_set & (1 << USR_PASSWD)) {
    result.hasPassword = true;
    result.password.assign(p + res.fields[USR_PASSWD].off,
                           res.fields[USR_PASSWD].len);
    result.password =
        util::percentDecode(result.password.begin(), result.password.end());
  }
  else {
    result.hasPassword = false;
    result.password = A2STR::NIL;
  }

  result.ipv6LiteralAddress = res.flags & USF_IPV6ADDR;
  return true;
}

std::string getFieldString(const uri_split_result& res, int field,
                           const char* base)
{
  if (res.field_set & (1 << field)) {
    return std::string(base + res.fields[field].off, res.fields[field].len);
  }
  return "";
}

std::string construct(const UriStruct& us)
{
  std::string res;
  res += us.protocol;
  res += "://";
  if (!us.username.empty()) {
    res += util::percentEncode(us.username);
    if (us.hasPassword) {
      res += ":";
      res += util::percentEncode(us.password);
    }
    res += "@";
  }
  if (us.ipv6LiteralAddress) {
    res += "[";
    res += us.host;
    res += "]";
  }
  else {
    res += us.host;
  }

  uint16_t defPort = getDefaultPort(us.protocol);
  if (us.port != 0 && defPort != us.port) {
    res += fmt(":%u", us.port);
  }

  res += us.dir;
  if (us.dir.empty() || us.dir[us.dir.size() - 1] != '/') {
    res += "/";
  }

  res += us.file;
  res += us.query;
  return res;
}

namespace {
enum { NPATH_START, NPATH_SLASH, NPATH_SDOT, NPATH_DDOT, NPATH_PATHCOMP };
}

std::string normalizePath(std::string path)
{
  auto begin = path.begin(), out = begin;
  int state = NPATH_START;
  bool startWithSlash = false;
  std::vector<int> range;
  // 32 is arbitrary
  range.reserve(32);

  for (auto in = begin, eoi = path.end(); in != eoi; ++in) {
    switch (state) {
    case NPATH_START:
      switch (*in) {
      case '.':
        state = NPATH_SDOT;
        range.push_back(in - begin);
        break;
      case '/':
        startWithSlash = true;
        state = NPATH_SLASH;
        break;
      default:
        state = NPATH_PATHCOMP;
        range.push_back(in - begin);
        break;
      }
      break;
    case NPATH_SLASH:
      switch (*in) {
      case '.':
        state = NPATH_SDOT;
        range.push_back(in - begin);
        break;
      case '/':
        // drop duplicate '/'
        break;
      default:
        state = NPATH_PATHCOMP;
        range.push_back(in - begin);
        break;
      }
      break;
    case NPATH_SDOT:
      switch (*in) {
      case '.':
        state = NPATH_DDOT;
        break;
      case '/':
        // drop path component '.'
        state = NPATH_SLASH;
        range.pop_back();
        break;
      default:
        state = NPATH_PATHCOMP;
        break;
      }
      break;
    case NPATH_DDOT:
      switch (*in) {
      case '/':
        // drop previous path component before '..'
        for (int i = 0; i < 3 && !range.empty(); ++i) {
          range.pop_back();
        }
        state = NPATH_SLASH;
        break;
      default:
        state = NPATH_PATHCOMP;
        break;
      }
      break;
    case NPATH_PATHCOMP:
      if (*in == '/') {
        range.push_back(in + 1 - begin);
        state = NPATH_SLASH;
      }
      break;
    }
  }

  switch (state) {
  case NPATH_SDOT:
    range.pop_back();
    break;
  case NPATH_DDOT:
    for (int i = 0; i < 3 && !range.empty(); ++i) {
      range.pop_back();
    }
    break;
  case NPATH_PATHCOMP:
    range.push_back(path.end() - begin);
    break;
  default:
    break;
  }

  if (startWithSlash) {
    ++out;
  }

  for (int i = 0; i < (int)range.size(); i += 2) {
    auto a = begin + range[i];
    auto b = begin + range[i + 1];
    if (a == out) {
      out = b;
    }
    else {
      out = std::copy(a, b, out);
    }
  }

  path.erase(out, path.end());
  return path;
}

namespace {
std::string joinPath(std::string basePath,
                     std::string::const_iterator newPathFirst,
                     std::string::const_iterator newPathLast)
{
  if (newPathFirst == newPathLast) {
    return basePath;
  }

  if (basePath.empty() || *newPathFirst == '/') {
    return normalizePath(std::string(newPathFirst, newPathLast));
  }

  if (basePath[basePath.size() - 1] == '/') {
    basePath.append(newPathFirst, newPathLast);
    return normalizePath(basePath);
  }

  basePath += "/";
  basePath.append(newPathFirst, newPathLast);
  return normalizePath(basePath);
}
} // namespace

std::string joinPath(const std::string& basePath, const std::string& newPath)
{
  return joinPath(basePath, newPath.begin(), newPath.end());
}

std::string joinUri(const std::string& baseUri, const std::string& uri)
{
  UriStruct us;
  if (parse(us, uri)) {
    return uri;
  }

  UriStruct bus;
  if (!parse(bus, baseUri)) {
    return uri;
  }

  std::string::const_iterator qend;
  for (qend = uri.begin(); qend != uri.end(); ++qend) {
    if (*qend == '#') {
      break;
    }
  }
  std::string::const_iterator end;
  for (end = uri.begin(); end != qend; ++end) {
    if (*end == '?') {
      break;
    }
  }
  std::string newpath = joinPath(bus.dir, uri.begin(), end);
  bus.dir.clear();
  bus.file.clear();
  bus.query.clear();
  std::string res = construct(bus);
  if (!newpath.empty()) {
    // res always ends with '/'. Since bus.dir also starts with '/',
    // regardless of uri, newpath always starts with '/'.
    res.append(newpath.begin() + 1, newpath.end());
  }
  res.append(end, qend);
  return res;
}

} // namespace uri

} // namespace aria2
