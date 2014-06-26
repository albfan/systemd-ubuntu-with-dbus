/* ANSI-C code produced by gperf version 3.0.4 */
/* Command-line: gperf  */
/* Computed positions: -k'7,11' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif


#include <stddef.h>
#include "conf-parser.h"
#include "networkd.h"
#include "net-util.h"
#include <string.h>

#define TOTAL_KEYWORDS 27
#define MIN_WORD_LENGTH 10
#define MAX_WORD_LENGTH 25
#define MIN_HASH_VALUE 10
#define MAX_HASH_VALUE 53
/* maximum key range = 44, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
network_network_gperf_hash (register const char *str, register unsigned int len)
{
  static const unsigned char asso_values[] =
    {
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54,  0, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 10, 54, 25,  0, 54,
      54, 20, 30, 54, 54,  0, 54,  5, 28, 54,
      23, 54, 54,  0,  0, 54, 15, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54,  5, 54,
      15, 20, 54, 54, 54,  0, 54,  0, 54, 54,
       0,  5, 54, 54, 54,  5,  0, 10, 54, 20,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
      54, 54, 54, 54, 54, 54
    };
  register int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[10]];
      /*FALLTHROUGH*/
      case 10:
      case 9:
      case 8:
      case 7:
        hval += asso_values[(unsigned char)str[6]];
        break;
    }
  return hval;
}

#ifdef __GNUC__
__inline
#if defined __GNUC_STDC_INLINE__ || defined __GNUC_GNU_INLINE__
__attribute__ ((__gnu_inline__))
#endif
#endif
const struct ConfigPerfItem *
network_network_gperf_lookup (register const char *str, register unsigned int len)
{
  static const struct ConfigPerfItem wordlist[] =
    {
      {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
      {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
      {(char*)0}, {(char*)0},
      {"Match.Type",                  config_parse_string,                0,                             offsetof(Network, match_type)},
      {"Network.DNS",                 config_parse_dns,                   0,                             offsetof(Network, dns)},
      {"Network.Bond",                config_parse_bond,                  0,                             offsetof(Network, bond)},
      {"DHCPv4.UseDNS",               config_parse_bool,                  0,                             offsetof(Network, dhcp_dns)},
      {"Network.Bridge",              config_parse_bridge,                0,                             offsetof(Network, bridge)},
      {"Network.Gateway",             config_parse_gateway,               0,                             0},
      {(char*)0},
      {"Route.Destination",           config_parse_destination,           0,                             0},
      {"DHCPv4.UseMTU",               config_parse_bool,                  0,                             offsetof(Network, dhcp_mtu)},
      {(char*)0},
      {"DHCPv4.UseDomainName",        config_parse_bool,                  0,                             offsetof(Network, dhcp_domainname)},
      {(char*)0},
      {"Network.VLAN",                config_parse_vlan,                  0,                             offsetof(Network, vlans)},
      {"Address.Label",               config_parse_label,                 0,                             0},
      {"Network.Description",         config_parse_string,                0,                             offsetof(Network, description)},
      {"DHCPv4.CriticalConnection",   config_parse_bool,                  0,                             offsetof(Network, dhcp_critical)},
      {(char*)0},
      {"Address.Broadcast",           config_parse_broadcast,             0,                             0},
      {"Match.Architecture",          config_parse_net_condition,         CONDITION_ARCHITECTURE,        offsetof(Network, match_arch)},
      {(char*)0},
      {"Network.Address",             config_parse_address,               0,                             0},
      {(char*)0},
      {"Match.Driver",                config_parse_string,                0,                             offsetof(Network, match_driver)},
      {"Match.Path",                  config_parse_string,                0,                             offsetof(Network, match_path)},
      {(char*)0},
      {"Address.Address",             config_parse_address,               0,                             0},
      {"Match.MACAddress",            config_parse_hwaddr,                0,                             offsetof(Network, match_mac)},
      {"Network.DHCP",                config_parse_bool,                  0,                             offsetof(Network, dhcp)},
      {"Match.Name",                  config_parse_ifname,                0,                             offsetof(Network, match_name)},
      {(char*)0},
      {"Match.Host",                  config_parse_net_condition,         CONDITION_HOST,                offsetof(Network, match_host)},
      {(char*)0}, {(char*)0},
      {"Match.KernelCommandLine",     config_parse_net_condition,         CONDITION_KERNEL_COMMAND_LINE, offsetof(Network, match_kernel)},
      {(char*)0},
      {"Match.Virtualization",        config_parse_net_condition,         CONDITION_VIRTUALIZATION,      offsetof(Network, match_virt)},
      {(char*)0}, {(char*)0},
      {"DHCPv4.UseHostname",          config_parse_bool,                  0,                             offsetof(Network, dhcp_hostname)},
      {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
      {"Route.Gateway",               config_parse_gateway,               0,                             0}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = network_network_gperf_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].section_and_lvalue;

          if (s && *str == *s && !strcmp (str + 1, s + 1))
            return &wordlist[key];
        }
    }
  return 0;
}
