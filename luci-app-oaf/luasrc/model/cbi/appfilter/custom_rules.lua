-- Custom rules (AdGuard Home syntax) editor for OpenAppFilter
local cbi = require "luci.cbi"
local fs = require "nixio.fs"

local RULES_FILE = "/etc/appfilter/custom_rules.txt"

local m, s

local function calc_rule_limit()
    local stat = fs.statvfs("/etc/appfilter") or fs.statvfs("/etc") or fs.statvfs("/")
    if not stat then
        return 0
    end

    local bavail = stat.bavail or stat.bfree or 0
    local bsize = stat.bsize or stat.frsize or 0
    local available_bytes = bavail * bsize

    return math.floor(available_bytes / (80 * 1000))
end

local rule_limit = calc_rule_limit()

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

local time_enable = s:option(Flag, "custom_rule_time_enable", translate("Run During Time Rules"),
    translate("If enabled, custom rules only take effect while the configured time rule is active."))
time_enable.rmempty = false

function time_enable.cfgvalue(self, section)
    local v = m.uci:get("appfilter", "global", "custom_rule_time_enable")
    if v == nil then
        return "0"
    end
    return v
end

function time_enable.write(self, section, value)
    cbi.Flag.write(self, section, value)
    os.execute("killall -SIGUSR1 oafd")
end

local max_num = s:option(DummyValue, "_custom_rule_max_num", translate("Rule Limit"))
max_num.rawhtml = true

function max_num.cfgvalue(self, section)
    m.uci:set("appfilter", "global", "custom_rule_max_num", tostring(rule_limit))
    return translate("Current limit") .. ": " .. tostring(rule_limit) .. " " ..
        translate("valid rules") .. ". " ..
        translate("It is calculated in this page as floor(available disk space bytes / (80 * 1000)); decimal units are used, so 1 MB = 1000 KB.")
end

local rules = s:option(TextValue, "_rules", translate("Rules (AdGuard Home Syntax)"),
    translate("One rule per line:<br/>" ..
        "||example.org^ - block example.org and all subdomains; * is allowed inside the domain<br/>" ..
        "@@||sub.example.org^ - allow sub.example.org and all its subdomains<br/>" ..
        "/REGEX/ - block domains matching the regular expression<br/>" ..
        "! comment or # comment - comment line<br/>" ..
        "<br/>" ..
        "Notes: custom rules are matched before the feature library; allow (ignore) rules have higher priority than block rules; " ..
        "regex supports |, grouping and character classes; the rule must not contain # ; or ,; " ..
        "the rule limit is calculated in this page as floor(available disk space bytes / (80 * 1000)), using decimal units where 1 MB = 1000 KB; changes take effect immediately after saving."))
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
    m.uci:set("appfilter", "global", "custom_rule_max_num", tostring(calc_rule_limit()))
    local f = io.open(RULES_FILE, "w")
    if f then
        f:write(value or "")
        f:close()
    end
    os.execute("killall -SIGUSR1 oafd")
    return true
end

return m
