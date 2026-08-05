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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <uci.h>
#include "appfilter_custom_rule.h"
#include "appfilter_netlink.h"
#include "appfilter_config.h"
#include "appfilter.h"
#include "utils.h"

/* af_nl_add_feature is defined in main.c */
extern int af_nl_add_feature(char *feature);

/* max length of host_url regex in kernel feature node
 * (oaf kernel MAX_HOST_URL_LEN = 128, reserve 4 bytes) */
#define MAX_CUSTOM_REGEX_LEN 124

enum {
    RULE_SKIP,
    RULE_DOMAIN_ALL,     /* ||domain^ : domain and all subdomains */
    RULE_DOMAIN_EXACT,   /* 127.0.0.1 domain / bare domain : exact domain */
    RULE_DOMAIN_SUB,     /* *.domain : subdomains only */
    RULE_REGEX,          /* /REGEX/ */
};

/* convert adguard domain to kernel regexp: '.' -> '\.', '*' -> '.*' */
static int escape_domain(char *dst, int dst_len, const char *domain)
{
    int len = 0;
    const char *p = domain;

    while (*p && len < dst_len - 1) {
        if (*p == '.') {
            if (len + 2 >= dst_len)
                break;
            dst[len++] = '\\';
            dst[len++] = '.';
        } else if (*p == '*') {
            if (len + 2 >= dst_len)
                break;
            dst[len++] = '.';
            dst[len++] = '*';
        } else {
            dst[len++] = *p;
        }
        p++;
    }
    dst[len] = '\0';
    return len;
}

/* build feature line and send to kernel via netlink:
 * block: appid custom_rules:[tcp;;;REGEX;;;0]
 * allow: appid custom_rules:[tcp;;;REGEX;;;1]
 */
static int add_custom_rule_feature(int appid, int ignore, const char *regex)
{
    char feature_buf[MAX_FEATURE_LINE_LEN] = {0};
    int ret;

    if (!regex || !regex[0])
        return -1;
    if (strlen(regex) > MAX_CUSTOM_REGEX_LEN) {
        LOG_WARN("custom rule regex too long, skip: %s\n", regex);
        return -1;
    }
    if (strstr(regex, "#") || strstr(regex, ";") || strstr(regex, ",") ||
        strstr(regex, "[") || strstr(regex, "]")) {
        LOG_WARN("custom rule contains invalid char(# ; , [ ]), skip: %s\n", regex);
        return -1;
    }
    snprintf(feature_buf, sizeof(feature_buf), "%d custom_rules:[tcp;;;%s;;;%d]",
             appid, regex, ignore);
    ret = af_nl_add_feature(feature_buf);
    if (ret < 0)
        LOG_ERROR("send custom rule feature failed: %s\n", feature_buf);
    else
        LOG_DEBUG("add custom rule: %s\n", feature_buf);
    return ret;
}

/* strip adguard modifiers('^...' '$...') and trailing whitespace from domain */
static void strip_domain_tail(char *domain)
{
    char *p = domain;

    while (*p) {
        if (*p == '^' || *p == '$' || *p == ' ' || *p == '\t' || *p == '\r')
            break;
        p++;
    }
    *p = '\0';
}

/* parse one adguard rule line, return rule type,
 * output ignore flag and pattern(domain or regex content) */
static int parse_rule_line(char *line, int *ignore, char *pattern, int pattern_len)
{
    char *p = line;
    int type = RULE_SKIP;

    str_trim(line);
    if (!line[0] || line[0] == '!' || line[0] == '#')
        return RULE_SKIP;
    /* adblock list header, e.g. [Adblock Plus 2.0] */
    if (line[0] == '[')
        return RULE_SKIP;

    *ignore = 0;
    if (strncmp(p, "@@", 2) == 0) {
        *ignore = 1;
        p += 2;
    }

    if (strncmp(p, "||", 2) == 0) {
        p += 2;
        type = RULE_DOMAIN_ALL;
    } else if (strncmp(p, "127.0.0.1 ", 10) == 0 || strncmp(p, "0.0.0.0 ", 8) == 0) {
        while (*p && !isspace(*p))
            p++;
        while (*p && isspace(*p))
            p++;
        type = RULE_DOMAIN_EXACT;
    } else if (strncmp(p, "*.", 2) == 0) {
        p += 2;
        type = RULE_DOMAIN_SUB;
    } else if (*p == '/') {
        char *end = strrchr(p + 1, '/');
        if (end && end > p + 1) {
            *end = '\0';
            p++;
            type = RULE_REGEX;
        } else {
            return RULE_SKIP;
        }
    } else {
        type = RULE_DOMAIN_EXACT;
    }

    snprintf(pattern, pattern_len, "%s", p);
    if (type != RULE_REGEX)
        strip_domain_tail(pattern);

    if (!pattern[0])
        return RULE_SKIP;
    return type;
}

/* add domain rule feature(s):
 * RULE_DOMAIN_ALL/EXACT : ^esc:?\d*$ (exact domain, host port supported)
 * RULE_DOMAIN_ALL/SUB   : \.esc:?\d*$ (all subdomains) */
static void add_domain_rule(int *appid, int ignore, const char *domain, int type)
{
    char esc[MAX_CUSTOM_REGEX_LEN] = {0};
    char reg[MAX_CUSTOM_REGEX_LEN] = {0};

    escape_domain(esc, sizeof(esc), domain);
    if (!esc[0])
        return;

    if (type == RULE_DOMAIN_ALL || type == RULE_DOMAIN_EXACT) {
        if (snprintf(reg, sizeof(reg), "^%s:?\\d*$", esc) < (int)sizeof(reg)) {
            add_custom_rule_feature(*appid, ignore, reg);
            (*appid)++;
        }
    }
    if (type == RULE_DOMAIN_ALL || type == RULE_DOMAIN_SUB) {
        if (snprintf(reg, sizeof(reg), "\\.%s:?\\d*$", esc) < (int)sizeof(reg)) {
            add_custom_rule_feature(*appid, ignore, reg);
            (*appid)++;
        }
    }
}

/* load custom rules from CUSTOM_RULES_FILE and push to kernel via netlink.
 * two passes: block rules first, then allow(ignore) rules.
 * kernel feature list uses list_add(head), later rules match first,
 * so allow rules must be loaded later to take priority over block rules. */
int load_custom_rules(void)
{
    FILE *fp = NULL;
    char line[MAX_CUSTOM_RULE_LINE_LEN] = {0};
    int pass;
    int appid = CUSTOM_RULE_APPID_BASE;
    int enable = 1;
    struct uci_context *ctx = NULL;

    /* custom_rule_enable, default enable */
    ctx = uci_alloc_context();
    if (ctx) {
        int v = af_uci_get_int_value(ctx, "appfilter.global.custom_rule_enable");
        if (v >= 0)
            enable = v;
        uci_free_context(ctx);
    }
    if (!enable) {
        LOG_WARN("custom rule is disabled, skip load\n");
        return 0;
    }

    fp = fopen(CUSTOM_RULES_FILE, "r");
    if (!fp) {
        LOG_DEBUG("custom rules file %s not found\n", CUSTOM_RULES_FILE);
        return 0;
    }

    for (pass = 0; pass < 2; pass++) {
        int rule_num = 0;
        fseek(fp, 0, SEEK_SET);
        while (fgets(line, sizeof(line), fp)) {
            int ignore = 0;
            int type;
            char pattern[MAX_CUSTOM_RULE_LINE_LEN] = {0};

            type = parse_rule_line(line, &ignore, pattern, sizeof(pattern));
            if (type == RULE_SKIP)
                continue;
            /* pass 0: block rules(ignore=0), pass 1: allow rules(ignore=1) */
            if (ignore != pass)
                continue;
            if (appid >= CUSTOM_RULE_APPID_BASE + MAX_CUSTOM_RULE_NUM) {
                LOG_WARN("custom rule appid exhausted(%d), ignore rest rules\n", appid);
                goto EXIT;
            }
            if (type == RULE_REGEX) {
                add_custom_rule_feature(appid, ignore, pattern);
                appid++;
            } else {
                add_domain_rule(&appid, ignore, pattern, type);
            }
            rule_num++;
        }
        LOG_DEBUG("custom rule pass %d: %d rules loaded\n", pass, rule_num);
    }
EXIT:
    fclose(fp);
    return 0;
}
