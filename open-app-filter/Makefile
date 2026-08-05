
include $(TOPDIR)/rules.mk

PKG_NAME:=appfilter
PKG_VERSION:=6.1.8
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
include $(INCLUDE_DIR)/package.mk
$(call Build/Prepare/Default)

TARGET_CFLAGS +=-Werror=implicit-function-declaration
define Package/appfilter
  SECTION:=TT Apps
  CATEGORY:=TT Apps
  DEPENDS:=+libubox +libubus +libuci +libpthread +libjson-c +libblobmsg-json
  TITLE:=OAF client and config service
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp -rf ./src/* $(PKG_BUILD_DIR)
endef

CFLAGS += $(CPPFLAGS)
define Build/Compile
	@echo "CC=$(TARGET_CC)"
    @echo "CPPFLAGS=$(TARGET_CPPFLAGS)"
    @echo "CFLAGS=$(TARGET_CFLAGS)"
    @echo "LDFLAGS=$(TARGET_LDFLAGS)"
    $(MAKE) -C $(PKG_BUILD_DIR) \
        CC="$(TARGET_CC)" \
        CPPFLAGS="$(TARGET_CPPFLAGS)" \
        CFLAGS="$(TARGET_CFLAGS)" \
        LDFLAGS="$(TARGET_LDFLAGS)" \
        LDLIBS="-lubox -lubus -luci -ljson-c -lblobmsg_json -lpthread -lm"
endef
# CFLAGS += $(CPPFLAGS)

#LDLIBS += \
#    -lubox \
#    -lubus \
#    -luci \
#    -ljson-c \
#    -lblobmsg_json \
#    -lpthread \
#    -lm
#define Build/Compile
#    $(MAKE) -C $(PKG_BUILD_DIR) \
#        CC="$(TARGET_CC)" \
#        CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS)" \
#        CPPFLAGS="$(TARGET_CPPFLAGS)" \
#        LDFLAGS="$(TARGET_LDFLAGS)" \
#        STAGING_DIR="$(STAGING_DIR)" \
#        all
#endef

define Build/Compile/Default

endef

define Package/appfilter/description
  openappfilter app
endef

define Package/appfilter/conffiles
/etc/config/appfilter
/etc/config/user_info
endef

define Package/appfilter/install
	$(INSTALL_DIR) $(1)/usr/bin $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/etc/appfilter
	$(INSTALL_DIR) $(1)/etc/config
	$(CP) ./files/*.cfg $(1)/etc/appfilter/ 
	$(INSTALL_BIN) ./files/appfilter.init $(1)/etc/init.d/appfilter
	$(INSTALL_BIN) ./files/oaf_rule $(1)/usr/bin
	$(INSTALL_BIN) ./files/gen_class.sh $(1)/usr/bin
	$(INSTALL_DATA) ./files/appfilter.config $(1)/etc/config/appfilter
	$(INSTALL_DATA) ./files/user_info.config $(1)/etc/config/user_info
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/oafd $(1)/usr/bin
	$(INSTALL_BIN) ./files/hnat.sh $(1)/usr/bin
endef


$(eval $(call BuildPackage,appfilter))

