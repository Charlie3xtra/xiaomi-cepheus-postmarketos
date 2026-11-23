"""Compile the actual modified C function with transport/completion stubs."""
from pathlib import Path
import re
import subprocess
import sys

root = Path(__file__).parent
driver = Path(sys.argv[1]) if len(sys.argv) > 1 else root / 'driver_src'
source = (driver / 'mac.c').read_text()
start = source.index('static int ath10k_install_key(')
end = source.index('\nstatic int ath10k_install_peer_wep_keys', start)
function = source[start:end]
wmi = (driver / 'wmi.h').read_text()
defines = '\n'.join(re.search(r'^#define ' + name + r'\s+[^\n]+', wmi, re.M).group()
                    for name in ('WMI_KEY_PAIRWISE', 'WMI_KEY_GROUP', 'WMI_KEY_TX_USAGE'))
prefix = r'''
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
typedef unsigned char u8;
typedef unsigned int u32;
enum set_key_cmd { SET_KEY, DISABLE_KEY };
struct ieee80211_key_conf { int unused; };
struct ath10k { int conf_mutex; int install_key_done; struct { bool no_pairwise_key_del_ind; } hw_params; };
struct ath10k_vif { struct ath10k *ar; bool nohwcrypt; };
static int send_result, send_calls, wait_calls;
static unsigned long wait_result;
#define HZ 100
#define lockdep_assert_held(p) ((void)(p))
#define reinit_completion(p) ((void)(p))
static int ath10k_send_key(struct ath10k_vif *a, struct ieee80211_key_conf *k,
                          enum set_key_cmd c, const u8 *m, u32 f)
{ (void)a; (void)k; (void)c; (void)m; (void)f; send_calls++; return send_result; }
static unsigned long wait_for_completion_timeout(int *p, unsigned long timeout)
{ (void)p; assert(timeout == 3 * HZ); wait_calls++; return wait_result; }
'''
suffix = r'''
static void check(bool quirk, bool nohwcrypt, enum set_key_cmd cmd, u32 flags,
                  int send_error, unsigned long indication, int expected, int sends, int waits)
{
 struct ath10k ar = { .hw_params.no_pairwise_key_del_ind = quirk };
 struct ath10k_vif vif = { .ar = &ar, .nohwcrypt = nohwcrypt };
 struct ieee80211_key_conf key = {0};
 send_result = send_error; wait_result = indication; send_calls = 0; wait_calls = 0;
 assert(ath10k_install_key(&vif, &key, cmd, NULL, flags) == expected);
 assert(send_calls == sends && wait_calls == waits);
}
int main(void)
{
 check(true, false, DISABLE_KEY, WMI_KEY_PAIRWISE, 0, 0, 0, 1, 0);
 check(true, false, DISABLE_KEY, WMI_KEY_PAIRWISE | WMI_KEY_TX_USAGE, 0, 0, 0, 1, 0);
 check(true, false, SET_KEY, WMI_KEY_PAIRWISE, 0, 0, -ETIMEDOUT, 1, 1);
 check(true, false, SET_KEY, WMI_KEY_PAIRWISE, 0, 1, 0, 1, 1);
 check(true, false, DISABLE_KEY, WMI_KEY_GROUP, 0, 0, -ETIMEDOUT, 1, 1);
 check(true, false, SET_KEY, WMI_KEY_GROUP, 0, 1, 0, 1, 1);
 check(false, false, DISABLE_KEY, WMI_KEY_PAIRWISE, 0, 0, -ETIMEDOUT, 1, 1);
 check(true, false, DISABLE_KEY, WMI_KEY_PAIRWISE, -EIO, 0, -EIO, 1, 0);
 check(true, true, SET_KEY, WMI_KEY_PAIRWISE, 0, 0, 1, 0, 0);
 puts("9 actual C key-path cases passed");
 return 0;
}
'''
test_source = root / 'key-path-test.c'
test_source.write_text(prefix + defines + '\n' + function + suffix)
subprocess.run(['gcc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                str(test_source), '-o', str(root / 'key-path-test')], check=True)
subprocess.run([str(root / 'key-path-test')], check=True)
