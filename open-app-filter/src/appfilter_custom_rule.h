/*
Copyright (C) 2020 Derry <destan19@126.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#ifndef __APPFILTER_CUSTOM_RULE_H__
#define __APPFILTER_CUSTOM_RULE_H__

/* custom rules (AdGuard Home syntax) file */
#define CUSTOM_RULES_FILE "/etc/appfilter/custom_rules.txt"
#define MAX_CUSTOM_RULE_LINE_LEN 256

/* must match AF_CUSTOM_RULE_APPID_BASE in oaf kernel module */
#define CUSTOM_RULE_APPID_BASE 30001

/* load custom rules from file and push to kernel via netlink */
int load_custom_rules(void);

/* returns whether custom rules are enabled */
int custom_rule_enabled(void);

/* returns whether custom rules should follow the configured time rule */
int custom_rule_time_mode_enabled(void);

#endif
