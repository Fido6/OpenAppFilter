-- Custom rules (AdGuard Home syntax) editor for OpenAppFilter
local cbi = require "luci.cbi"

local RULES_FILE = "/etc/appfilter/custom_rules.txt"

local m, s

m = Map("appfilter", translate("Custom Rules"),
    translate("Custom rules follow the AdGuard Home syntax. They take effect immediately after saving, no feature library update is required. Custom rules are matched before the feature library, so they can override library rules."))

s = m:section(NamedSection, "global", "global", translate("General"))
s.anonymous = true
s.addremove = false

local enable = s:option(Flag, "custom_rule_enable", translate("Enable Custom Rules"),
    translate("If disabled, all custom rules will not take effect."))
enable.rmempty = false

function enable.cfgvalue(self, section)
    local v = m.uci:get("appfilter", "global", "custom_rule_enable")
    if v == nil then
        return "1"
    end
    return v
end

function enable.write(self, section, value)
    cbi.Flag.write(self, section, value)
    os.execute("killall -SIGUSR1 oafd")
end

local rules = s:option(TextValue, "_rules", translate("Rules (AdGuard Home Syntax)"),
    translate("One rule per line:<br/>" ..
        "||example.org^ - block example.org and all subdomains<br/>" ..
        "@@||sub.example.org^ - allow sub.example.org and all its subdomains<br/>" ..
        "/REGEX/ - block domains matching the regular expression<br/>" ..
        "@@/REGEX/ - allow domains matching the regular expression<br/>" ..
        "! comment or # comment - comment line<br/>" ..
        "<br/>" ..
        "Notes: custom rules are matched before the feature library; allow (ignore) rules have higher priority than block rules; " ..
        "regex does not support | ( ) { } and the rule must not contain # ; , [ ]; " ..
        "up to 500 rules are supported; changes take effect immediately after saving."))
rules.rows = 22
rules.rmempty = false
rules.wrap = "off"

function rules.cfgvalue(self, section)
    local f = io.open(RULES_FILE, "r")
    if not f then
        return ""
    end
    local content = f:read("*a")
    f:close()
    return content
end

function rules.write(self, section, value)
    os.execute("mkdir -p /etc/appfilter")
    local f = io.open(RULES_FILE, "w")
    if f then
        f:write(value or "")
        f:close()
    end
    os.execute("killall -SIGUSR1 oafd")
    return true
end

return m
