#include "sca_ble_peripheral.hpp"

#include <gio/gio.h>
#include <glib.h>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <iomanip>

// ===== 원본 전역들을 내부 static으로 =====
static std::string g_service_uuid;
static std::string g_local_name = "SCA-CAR";
static std::string g_expected_token = "ACCESS";
static int  g_timeout_sec = 30;
static bool g_require_encrypt = false;

static bool g_done = false;
static bool g_ok = false;

static const char* APP_PATH = "/com/sca/app";
static const char* SERVICE_PATH = "/com/sca/app/service0";
static const char* CHAR_PATH = "/com/sca/app/service0/char0";
static const char* ADV_PATH = "/com/sca/app/adv0";

static const char* SERVICE_IF_XML = R"XML(
<node>
  <interface name="org.bluez.GattService1">
    <property name="UUID" type="s" access="read"/>
    <property name="Primary" type="b" access="read"/>
    <property name="Includes" type="ao" access="read"/>
  </interface>
</node>
)XML";

static const char* CHAR_IF_XML = R"XML(
<node>
  <interface name="org.bluez.GattCharacteristic1">
    <method name="ReadValue">
      <arg type="a{sv}" name="Options" direction="in"/>
      <arg type="ay"    name="Value"   direction="out"/>
    </method>
    <method name="WriteValue">
      <arg type="ay"    name="Value"   direction="in"/>
      <arg type="a{sv}" name="Options" direction="in"/>
    </method>
    <property name="UUID"     type="s"  access="read"/>
    <property name="Service"  type="o"  access="read"/>
    <property name="Flags"    type="as" access="read"/>
    <property name="Descriptors" type="ao" access="read"/>
  </interface>
</node>
)XML";

static const char* ADV_IF_XML = R"XML(
<node>
  <interface name="org.bluez.LEAdvertisement1">
    <method name="Release"/>
    <property name="Type"         type="s"  access="read"/>
    <property name="ServiceUUIDs" type="as" access="read"/>
    <property name="Includes"     type="as" access="read"/>
  </interface>
</node>
)XML";

static const char* OBJMGR_IF_XML = R"XML(
<node>
  <interface name="org.freedesktop.DBus.ObjectManager">
    <method name="GetManagedObjects">
      <arg type="a{oa{sa{sv}}}" direction="out"/>
    </method>
  </interface>
</node>
)XML";

// ===== ObjectManager 응답 =====
static void objmgr_method_call(GDBusConnection* c, const gchar* sender,
    const gchar* obj_path, const gchar* iface, const gchar* method,
    GVariant* params, GDBusMethodInvocation* inv, gpointer) {

    if (g_strcmp0(iface, "org.freedesktop.DBus.ObjectManager") == 0 &&
        g_strcmp0(method, "GetManagedObjects") == 0) {

        GVariantBuilder root;
        g_variant_builder_init(&root, G_VARIANT_TYPE("a{oa{sa{sv}}}"));

        // service
        {
            GVariantBuilder ifmap;  g_variant_builder_init(&ifmap, G_VARIANT_TYPE("a{sa{sv}}"));
            GVariantBuilder props;  g_variant_builder_init(&props, G_VARIANT_TYPE("a{sv}"));

            g_variant_builder_add(&props, "{sv}", "UUID", g_variant_new_string(g_service_uuid.c_str()));
            g_variant_builder_add(&props, "{sv}", "Primary", g_variant_new_boolean(TRUE));
            g_variant_builder_add(&props, "{sv}", "Includes", g_variant_new("ao", NULL));
            g_variant_builder_add(&ifmap, "{s@a{sv}}", "org.bluez.GattService1", g_variant_builder_end(&props));
            g_variant_builder_add(&root, "{o@a{sa{sv}}}", SERVICE_PATH, g_variant_builder_end(&ifmap));
        }

        // characteristic
        {
            GVariantBuilder ifmap;  g_variant_builder_init(&ifmap, G_VARIANT_TYPE("a{sa{sv}}"));
            GVariantBuilder props;  g_variant_builder_init(&props, G_VARIANT_TYPE("a{sv}"));

            g_variant_builder_add(&props, "{sv}", "UUID", g_variant_new_string("c0de0001-0000-1000-8000-000000000001"));
            g_variant_builder_add(&props, "{sv}", "Service", g_variant_new_object_path(SERVICE_PATH));
            const char* flags_arr[] = { "write", "write-without-response", NULL };
            g_variant_builder_add(&props, "{sv}", "Flags", g_variant_new_strv(flags_arr, -1));
            g_variant_builder_add(&props, "{sv}", "Descriptors", g_variant_new("ao", NULL));

            g_variant_builder_add(&ifmap, "{s@a{sv}}", "org.bluez.GattCharacteristic1", g_variant_builder_end(&props));
            g_variant_builder_add(&root, "{o@a{sa{sv}}}", CHAR_PATH, g_variant_builder_end(&ifmap));
        }

        GVariant* dict = g_variant_builder_end(&root);
        g_dbus_method_invocation_return_value(inv, g_variant_new("(@a{oa{sa{sv}}})", dict));
        return;
    }

    g_dbus_method_invocation_return_dbus_error(inv, "org.freedesktop.DBus.Error.UnknownMethod", "Unknown method");
}

// ===== 어댑터 찾기 =====
static std::string get_adapter_path(GDBusConnection* conn) {
    GError* err = nullptr;
    GVariant* ret = g_dbus_connection_call_sync(
        conn, "org.bluez", "/",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects",
        nullptr, nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &err);

    if (!ret) { if (err) g_error_free(err); std::cerr << "[ERR] No org.bluez.Adapter1 found\n"; std::exit(2); }

    if (g_variant_is_of_type(ret, G_VARIANT_TYPE_TUPLE)) {
        GVariant* dict = g_variant_get_child_value(ret, 0);
        GVariantIter* i = nullptr; g_variant_get(dict, "a{oa{sa{sv}}}", &i);
        std::string adapterPath;
        const gchar* objpath = nullptr; GVariant* ifaces = nullptr;
        while (g_variant_iter_loop(i, "{&o@a{sa{sv}}}", &objpath, &ifaces)) {
            GVariantIter* ii = nullptr; const gchar* ifname = nullptr; GVariant* props = nullptr;
            g_variant_get(ifaces, "a{sa{sv}}", &ii);
            while (g_variant_iter_loop(ii, "{&s@a{sv}}", &ifname, &props)) {
                if (g_strcmp0(ifname, "org.bluez.Adapter1") == 0) adapterPath = objpath;
            }
            if (ii) g_variant_iter_free(ii);
        }
        if (i) g_variant_iter_free(i);
        g_variant_unref(dict);
        g_variant_unref(ret);
        if (!adapterPath.empty()) return adapterPath;
    }
    std::cerr << "[ERR] No org.bluez.Adapter1 found\n"; std::exit(2);
}

// ===== Properties Getters =====
static GVariant* service_get(GDBusConnection*, const gchar*, const gchar*, const gchar*,
    const gchar* prop, GError**, gpointer) {
    if (!g_strcmp0(prop, "UUID"))     return g_variant_new_string(g_service_uuid.c_str());
    if (!g_strcmp0(prop, "Primary"))  return g_variant_new_boolean(TRUE);
    if (!g_strcmp0(prop, "Includes")) return g_variant_new("ao", nullptr);
    if (!g_strcmp0(prop, "Device"))   return g_variant_new_object_path("/");
    return nullptr;
}
static GVariant* char_get(GDBusConnection*, const gchar*, const gchar*, const gchar*,
    const gchar* prop, GError**, gpointer) {
    if (!g_strcmp0(prop, "UUID"))    return g_variant_new_string("c0de0001-0000-1000-8000-000000000001");
    if (!g_strcmp0(prop, "Service")) return g_variant_new_object_path(SERVICE_PATH);
    if (!g_strcmp0(prop, "Value"))   return g_variant_new_from_data(G_VARIANT_TYPE("ay"), "", 0, TRUE, nullptr, nullptr);
    if (!g_strcmp0(prop, "Flags")) {
        if (g_require_encrypt) {
            const gchar* flags[] = { "write","encrypt-write",nullptr };
            return g_variant_new_strv(flags, -1);
        }
        else {
            const gchar* flags[] = { "write",nullptr };
            return g_variant_new_strv(flags, -1);
        }
    }
    return nullptr;
}
static GVariant* adv_get(GDBusConnection*, const gchar*, const gchar*, const gchar*,
    const gchar* prop, GError**, gpointer) {
    if (g_strcmp0(prop, "Type") == 0)
        return g_variant_new_string("peripheral");
    if (g_strcmp0(prop, "ServiceUUIDs") == 0) {
        const char* u = g_service_uuid.c_str();
        return g_variant_new_strv(&u, 1);
    }
    if (g_strcmp0(prop, "Includes") == 0) {
        const gchar* inc[] = { "local-name", nullptr };
        return g_variant_new_strv(inc, -1);
    }
    return nullptr;
}

// ===== Method Handlers =====
static void char_method_call(GDBusConnection*, const gchar*, const gchar*, const gchar*,
    const gchar* method, GVariant* params, GDBusMethodInvocation* inv, gpointer) {
    if (!g_strcmp0(method, "WriteValue")) {
        GVariant* value = nullptr, * options = nullptr;
        g_variant_get(params, "(@ay@a{sv})", &value, &options);
        gsize n = 0; const guchar* bytes = (const guchar*)g_variant_get_fixed_array(value, &n, sizeof(guchar));
        std::string data((const char*)bytes, (const char*)bytes + n);

        std::cout << "[BLE] WriteValue len=" << n << " data=\"";
        for (gsize i = 0; i < n; i++) {
            unsigned char c = bytes[i];
            if (c >= 32 && c < 127) std::cout << (char)c;
            else std::cout << "\\x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)c << std::dec;
        }
        std::cout << "\"\n";

        g_done = true;
        if (data == g_expected_token) {
            g_ok = true;  g_dbus_method_invocation_return_value(inv, nullptr);
        }
        else {
            g_ok = false; g_dbus_method_invocation_return_dbus_error(inv, "com.sca.Error.InvalidPayload", "Invalid payload");
        }
        if (options) g_variant_unref(options);
        if (value) g_variant_unref(value);
        return;
    }
    if (!g_strcmp0(method, "ReadValue")) {
        GVariant* options = nullptr; g_variant_get(params, "(@a{sv})", &options); if (options) g_variant_unref(options);
        GVariant* empty = g_variant_new_from_data(G_VARIANT_TYPE("ay"), "", 0, TRUE, nullptr, nullptr);
        g_dbus_method_invocation_return_value(inv, g_variant_new_tuple(&empty, 1));
        return;
    }
    g_dbus_method_invocation_return_dbus_error(inv, "org.freedesktop.DBus.Error.UnknownMethod", "Unknown method");
}
static void adv_method_call(GDBusConnection*, const gchar*, const gchar*,
    const gchar* iface, const gchar* method,
    GVariant*, GDBusMethodInvocation* inv, gpointer) {
    if (!g_strcmp0(iface, "org.bluez.LEAdvertisement1") && !g_strcmp0(method, "Release")) {
        g_dbus_method_invocation_return_value(inv, NULL);
        return;
    }
    g_dbus_method_invocation_return_dbus_error(inv, "org.freedesktop.DBus.Error.UnknownMethod", "Unknown");
}
static bool call_set(GDBusConnection* conn, const std::string& objpath,
    const char* iface, const char* prop, GVariant* value,
    GError** err_out = nullptr) {
    GError* err = nullptr;
    GVariant* r = g_dbus_connection_call_sync(
        conn, "org.bluez", objpath.c_str(),
        "org.freedesktop.DBus.Properties", "Set",
        g_variant_new("(ssv)", iface, prop, value),
        nullptr, G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, &err);
    if (!r) { if (err_out) *err_out = err; else if (err) g_error_free(err); return false; }
    g_variant_unref(r);
    return true;
}

// ===== VTABLEs =====
static const GDBusInterfaceVTable SERVICE_VTABLE = { nullptr, service_get, nullptr };
static const GDBusInterfaceVTable CHAR_VTABLE = { char_method_call, char_get, nullptr };
static const GDBusInterfaceVTable ADV_VTABLE = { adv_method_call,  adv_get,  nullptr };
static const GDBusInterfaceVTable OBJMGR_VTABLE = { objmgr_method_call, nullptr, nullptr };

// ===== 유틸: 12-hex로 UUID 구성 =====
static std::string build_service_uuid_last12(const std::string& last12) {
    if (last12.size() != 12) {
        std::cerr << "[ERR] --hash must be 12 hex (last UUID block)\n";
        return {};
    }
    std::string up = last12;
    for (auto& c : up) c = (char)std::toupper((unsigned char)c);
    return std::string("12345678-0000-1000-8000-") + up;
}

// ===== 공개 함수(핵심): 12-hex 사용 =====
int run_ble_peripheral_12hash(const BleOptions& opt) {
    g_done = false; g_ok = false;

    g_service_uuid = build_service_uuid_last12(opt.hash_last12);
    if (g_service_uuid.empty()) return 1;

    g_local_name = opt.local_name;
    g_timeout_sec = opt.timeout_sec;
    g_require_encrypt = opt.require_encrypt;
    g_expected_token = opt.expected_token;

    GError* err = nullptr;
    GDBusConnection* conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &err);
    if (!conn) { if (err) g_error_free(err); return 2; }

    std::string adapterPath = get_adapter_path(conn);

    // 어댑터 설정
    call_set(conn, adapterPath, "org.bluez.Adapter1", "Powered", g_variant_new_boolean(TRUE));
    call_set(conn, adapterPath, "org.bluez.Adapter1", "Discoverable", g_variant_new_boolean(TRUE));
    call_set(conn, adapterPath, "org.bluez.Adapter1", "Pairable", g_variant_new_boolean(TRUE));
    call_set(conn, adapterPath, "org.bluez.Adapter1", "Alias", g_variant_new_string(g_local_name.c_str()));

    // XML 파싱
    GDBusNodeInfo* service_node = g_dbus_node_info_new_for_xml(SERVICE_IF_XML, &err);
    if (!service_node) { if (err) g_error_free(err); return 2; }
    GDBusNodeInfo* char_node = g_dbus_node_info_new_for_xml(CHAR_IF_XML, &err);
    if (!char_node) { if (err) g_error_free(err); g_dbus_node_info_unref(service_node); return 2; }
    GDBusNodeInfo* adv_node = g_dbus_node_info_new_for_xml(ADV_IF_XML, &err);
    if (!adv_node) { if (err) g_error_free(err); g_dbus_node_info_unref(char_node); g_dbus_node_info_unref(service_node); return 2; }
    GDBusNodeInfo* objmgr_node = g_dbus_node_info_new_for_xml(OBJMGR_IF_XML, &err);
    if (!objmgr_node) { if (err) g_error_free(err); g_dbus_node_info_unref(adv_node); g_dbus_node_info_unref(char_node); g_dbus_node_info_unref(service_node); return 2; }

    // 객체 등록
    guint reg_service = g_dbus_connection_register_object(conn, SERVICE_PATH, service_node->interfaces[0], &SERVICE_VTABLE, nullptr, nullptr, &err);
    if (!reg_service) { if (err) g_error_free(err); goto cleanup_nodes; }
    guint reg_char = g_dbus_connection_register_object(conn, CHAR_PATH, char_node->interfaces[0], &CHAR_VTABLE, nullptr, nullptr, &err);
    if (!reg_char) { if (err) g_error_free(err); goto cleanup_service; }
    guint reg_adv = g_dbus_connection_register_object(conn, ADV_PATH, adv_node->interfaces[0], &ADV_VTABLE, nullptr, nullptr, &err);
    if (!reg_adv) { if (err) g_error_free(err); goto cleanup_char; }
    guint reg_objmgr = g_dbus_connection_register_object(conn, APP_PATH, objmgr_node->interfaces[0], &OBJMGR_VTABLE, nullptr, nullptr, &err);
    if (!reg_objmgr) { if (err) g_error_free(err); goto cleanup_adv; }

    // RegisterApplication
    {
        bool app_registered = false;
        GVariant* options = g_variant_new_array(G_VARIANT_TYPE("{sv}"), nullptr, 0);
        g_dbus_connection_call(
            conn, "org.bluez", adapterPath.c_str(),
            "org.bluez.GattManager1", "RegisterApplication",
            g_variant_new("(o@a{sv})", APP_PATH, options),
            nullptr, G_DBUS_CALL_FLAGS_NONE, 5000, nullptr,
            [](GObject* src, GAsyncResult* res, gpointer user_data) {
                GError* e = nullptr; GVariant* r = g_dbus_connection_call_finish(G_DBUS_CONNECTION(src), res, &e);
                bool* ok = static_cast<bool*>(user_data);
                if (!r) { if (e) g_error_free(e); *ok = false; }
                else { g_variant_unref(r); *ok = true; }
            }, &app_registered
        );
        while (!app_registered) g_main_context_iteration(nullptr, TRUE);
    }

    // RegisterAdvertisement (async)
    {
        struct AdvRegState { bool ok = false; bool fail = false; } st;
        GVariant* opts = g_variant_new_array(G_VARIANT_TYPE("{sv}"), nullptr, 0);
        g_dbus_connection_call(
            conn, "org.bluez", adapterPath.c_str(),
            "org.bluez.LEAdvertisingManager1", "RegisterAdvertisement",
            g_variant_new("(o@a{sv})", ADV_PATH, opts),
            nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr,
            [](GObject* src, GAsyncResult* res, gpointer user_data) {
                GError* e = nullptr; GVariant* r = g_dbus_connection_call_finish(G_DBUS_CONNECTION(src), res, &e);
                auto* st = static_cast<AdvRegState*>(user_data);
                if (!r) { if (e) g_error_free(e); st->fail = true; }
                else { g_variant_unref(r); st->ok = true; }
            }, &st
        );
        while (!st.ok && !st.fail) g_main_context_iteration(nullptr, TRUE);
        if (!st.ok) { goto cleanup_all; }
    }

    std::cout << "[BLE] Advertising service " << g_service_uuid
        << " name=" << g_local_name
        << " expect=\"" << g_expected_token << "\""
        << " encrypt=" << (g_require_encrypt ? "on" : "off")
        << " timeout=" << g_timeout_sec << "s\n";

    // 이벤트 루프(폴링)
    {
        int elapsed_ms = 0;
        while (!g_done && elapsed_ms < g_timeout_sec * 1000) {
            g_main_context_iteration(nullptr, FALSE);
            g_usleep(200 * 1000);
            elapsed_ms += 200;
        }
    }

    // Unregister & 정리
    g_dbus_connection_call_sync(conn, "org.bluez", adapterPath.c_str(),
        "org.bluez.LEAdvertisingManager1", "UnregisterAdvertisement",
        g_variant_new("(o)", ADV_PATH), nullptr, G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, nullptr);

cleanup_all:
    g_dbus_connection_unregister_object(conn, reg_objmgr);
cleanup_adv:
    g_dbus_connection_unregister_object(conn, reg_adv);
cleanup_char:
    g_dbus_connection_unregister_object(conn, reg_char);
cleanup_service:
    g_dbus_connection_unregister_object(conn, reg_service);
cleanup_nodes:
    g_dbus_node_info_unref(objmgr_node);
    g_dbus_node_info_unref(adv_node);
    g_dbus_node_info_unref(char_node);
    g_dbus_node_info_unref(service_node);

    return (g_done && g_ok) ? 0 : 1;
}

// ===== 16-hex 편의 API =====
int run_ble_peripheral_16hash(const std::string& hash16,
    const std::string& local_name,
    int timeout_sec,
    bool require_encrypt,
    const std::string& expected_token) {
    // 정책: 앞 12자를 사용(필요하면 여기서 자르는 규칙 변경)
    std::string h = hash16;
    for (auto& c : h) c = (char)std::toupper((unsigned char)c);
    if (h.size() < 12) return 1;
    BleOptions opt;
    opt.hash_last12 = h.substr(0, 12);
    opt.local_name = local_name;
    opt.timeout_sec = timeout_sec;
    opt.require_encrypt = require_encrypt;
    opt.expected_token = expected_token;
    return run_ble_peripheral_12hash(opt);
}
