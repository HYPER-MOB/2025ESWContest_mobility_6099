// sca_ble_peripheral.cpp
// Build:
//   g++ -std=c++17 sca_ble_peripheral.cpp -o sca_ble_peripheral `pkg-config --cflags --libs glib-2.0 gio-2.0`
// Run (example):
//   sudo ./sca_ble_peripheral --hash A1B2C3D4E5F60708 --name SCA-CAR --timeout 30 --token ACCESS --require-encrypt 0

#include <gio/gio.h>
#include <glib.h>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <iomanip>

static const char* BASE_UUID_PREFIX = "12345678-0000-1000-8000-";
static const char* ACCESS_CHAR_UUID = "c0de0001-0000-1000-8000-000000000001";

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
    <property name="Device" type="o" access="read"/>
    <property name="Primary" type="b" access="read"/>
    <property name="Includes" type="ao" access="read"/>
  </interface>
</node>
)XML";

static const char* CHAR_IF_XML = R"XML(
<node>
  <interface name="org.bluez.GattCharacteristic1">
    <method name="ReadValue">
      <arg name="Options" type="a{sv}" direction="in"/>
      <arg name="Value"   type="ay"    direction="out"/>
    </method>
    <method name="WriteValue">
      <arg name="Value"   type="ay"    direction="in"/>
      <arg name="Options" type="a{sv}" direction="in"/>
    </method>
    <property name="UUID"    type="s"  access="read"/>
    <property name="Service" type="o"  access="read"/>
    <property name="Value"   type="ay" access="read"/>
    <property name="Flags"   type="as" access="read"/>
  </interface>
</node>
)XML";

static const char* ADV_IF_XML = R"XML(
<node>
  <interface name="org.bluez.LEAdvertisement1">
    <method name="Release"/>
    <property name="Type"          type="s"  access="read"/>
    <property name="ServiceUUIDs"  type="as" access="read"/>
    <property name="LocalName"     type="s"  access="read"/>
    <property name="Discoverable"  type="b"  access="read"/>
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

static void objmgr_method_call(GDBusConnection* c, const gchar* sender,
    const gchar* obj_path, const gchar* iface, const gchar* method,
    GVariant* params, GDBusMethodInvocation* inv, gpointer user_data)
{
    if (g_strcmp0(iface, "org.freedesktop.DBus.ObjectManager") == 0 &&
        g_strcmp0(method, "GetManagedObjects") == 0) {
        std::cerr << "[STEP] get method \n";

        // a{oa{sa{sv}}} 빌드
        GVariantBuilder root;
        g_variant_builder_init(&root, G_VARIANT_TYPE("a{oa{sa{sv}}}"));

        // --- service 오브젝트 ---
        {
            GVariantBuilder ifmap; g_variant_builder_init(&ifmap, G_VARIANT_TYPE("a{sa{sv}}"));
            GVariantBuilder props; g_variant_builder_init(&props, G_VARIANT_TYPE("a{sv}"));
            g_variant_builder_add(&props, "{sv}", "UUID", g_variant_new_string(g_service_uuid.c_str()));
            g_variant_builder_add(&props, "{sv}", "Primary", g_variant_new_boolean(TRUE));
            g_variant_builder_add(&props, "{sv}", "Includes", g_variant_new("ao", NULL));
            // Device는 optional; 넣어도 무방
            g_variant_builder_add(&ifmap, "{s@a{sv}}", "org.bluez.GattService1", g_variant_builder_end(&props));
            g_variant_builder_add(&root, "{o@a{sa{sv}}}", SERVICE_PATH, g_variant_builder_end(&ifmap));
        }

        // --- characteristic 오브젝트 ---
        {
            GVariantBuilder ifmap; g_variant_builder_init(&ifmap, G_VARIANT_TYPE("a{sa{sv}}"));
            GVariantBuilder props; g_variant_builder_init(&props, G_VARIANT_TYPE("a{sv}"));
            g_variant_builder_add(&props, "{sv}", "UUID", g_variant_new_string(ACCESS_CHAR_UUID));
            g_variant_builder_add(&props, "{sv}", "Service", g_variant_new_object_path(SERVICE_PATH));
            // 빈 값
            g_variant_builder_add(&props, "{sv}", "Value", g_variant_new_from_data(G_VARIANT_TYPE("ay"), "", 0, TRUE, NULL, NULL));
            // Flags(as)
            const gchar* flags[] = { "write","write-without-response", NULL };
            g_variant_builder_add(&props, "{sv}", "Flags", g_variant_new_strv(flags, -1));
            g_variant_builder_add(&ifmap, "{s@a{sv}}", "org.bluez.GattCharacteristic1", g_variant_builder_end(&props));
            g_variant_builder_add(&root, "{o@a{sa{sv}}}", CHAR_PATH, g_variant_builder_end(&ifmap));
        }

        g_dbus_method_invocation_return_value(inv,
            g_variant_new("(a{oa{sa{sv}}})", g_variant_builder_end(&root)));
        return;
    }

    g_dbus_method_invocation_return_dbus_error(inv,
        "org.freedesktop.DBus.Error.UnknownMethod", "Unknown method");
}

static const GDBusInterfaceVTable OBJMGR_VTABLE = {
  objmgr_method_call, nullptr, nullptr
};

// ---------- utils ----------
static std::string build_service_uuid(const std::string& hash16) {
    if (hash16.size() != 16) { std::cerr << "[ERR] --hash must be 16 hex\n"; std::exit(2); }
    std::string up = hash16; for (auto& c : up) c = (char)std::toupper((unsigned char)c);
    return std::string(BASE_UUID_PREFIX) + up;
}

// 안전 파싱: GetManagedObjects의 리턴은 "a{oa{sa{sv}}}" (튜플 아님!)
static std::string get_adapter_path(GDBusConnection* conn) {

    GError* err = nullptr;
    std::cerr << "[STEP] GetManagedObjects at " << '/' << "\n";
    GVariant* ret = g_dbus_connection_call_sync(
        conn, "org.bluez", "/",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects",
        /*parameters*/ nullptr, /*reply_type*/ nullptr,
        G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &err);

    if (!ret) {
        std::cerr << "  -> call failed: " << (err ? err->message : "unknown") << "\n";
        if (err) g_error_free(err);
        std::cerr << "[ERR] No org.bluez.Adapter1 found on any path\n";
        std::exit(2);
    }

    GVariant* dict = nullptr;

    if (g_variant_is_of_type(ret, G_VARIANT_TYPE_TUPLE)) {
        g_variant_n_children(ret);

        dict = g_variant_get_child_value(ret, 0);

        GVariantIter* i = nullptr;
        g_variant_get(dict, "a{oa{sa{sv}}}", &i);

        std::string adapterPath;
        const gchar* objpath = nullptr;
        GVariant* ifaces = nullptr;

        size_t obj_idx = 0;
        while (g_variant_iter_loop(i, "{&o@a{sa{sv}}}", &objpath, &ifaces)) {

            GVariantIter* ii = nullptr;
            const gchar* ifname = nullptr;
            GVariant* props = nullptr;

            g_variant_get(ifaces, "a{sa{sv}}", &ii);
            size_t if_idx = 0;
            while (g_variant_iter_loop(ii, "{&s@a{sv}}", &ifname, &props)) {
                if (g_strcmp0(ifname, "org.bluez.Adapter1") == 0) {
                    std::cerr << "  [DBG]   -> Adapter1 FOUND at " << objpath << "\n";
                    adapterPath = objpath;
                }
                ++if_idx;
            }
            if (ii) g_variant_iter_free(ii);
            ++obj_idx;
        }

        if (i) g_variant_iter_free(i);
        g_variant_unref(dict);
        g_variant_unref(ret);

        if (!adapterPath.empty()) return adapterPath;
        std::cerr << "  -> Adapter1 not found in tuple result\n";
        // return "" 또는 다음 후보 처리
    }
    std::cerr << "[ERR] No org.bluez.Adapter1 found on any path\n";
    std::exit(2);
}

// ---------- property getters ----------
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
    if (!g_strcmp0(prop, "UUID"))    return g_variant_new_string(ACCESS_CHAR_UUID);
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
    if (!g_strcmp0(prop, "Type"))         return g_variant_new_string("peripheral");
    if (!g_strcmp0(prop, "LocalName"))    return g_variant_new_string(g_local_name.c_str());
    if (!g_strcmp0(prop, "Discoverable")) return g_variant_new_boolean(TRUE);
    if (!g_strcmp0(prop, "ServiceUUIDs")) {
        GVariantBuilder b; g_variant_builder_init(&b, G_VARIANT_TYPE("as"));
        g_variant_builder_add(&b, "s", g_service_uuid.c_str());
        return g_variant_builder_end(&b);
    }
    return nullptr;
}

// ---------- method handlers ----------
static void char_method_call(GDBusConnection*, const gchar*, const gchar*, const gchar*,
    const gchar* method, GVariant* params,
    GDBusMethodInvocation* inv, gpointer) {
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
            g_ok = true;
            g_dbus_method_invocation_return_value(inv, nullptr);
        }
        else {
            g_ok = false;
            g_dbus_method_invocation_return_dbus_error(inv, "com.sca.Error.InvalidPayload", "Invalid payload");
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
static void adv_method_call(GDBusConnection*, const gchar*, const gchar*, const gchar*,
    const gchar* method, GVariant*, GDBusMethodInvocation* inv, gpointer) {
    if (!g_strcmp0(method, "Release")) {
        g_dbus_method_invocation_return_value(inv, nullptr);
        return;
    }
    g_dbus_method_invocation_return_dbus_error(inv, "org.freedesktop.DBus.Error.UnknownMethod", "Unknown method");
}

// ---------- vtables (정적) ----------
static const GDBusInterfaceVTable SERVICE_VTABLE = { nullptr, service_get, nullptr };
static const GDBusInterfaceVTable CHAR_VTABLE = { char_method_call, char_get, nullptr };
static const GDBusInterfaceVTable ADV_VTABLE = { adv_method_call,  adv_get,  nullptr };

int main(int argc, char** argv) {
    std::string hash, name = "SCA-CAR"; int timeoutSec = 30; std::string token = "ACCESS"; bool requireEncrypt = false;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--hash") && i + 1 < argc) hash = argv[++i];
        else if (!strcmp(argv[i], "--name") && i + 1 < argc) name = argv[++i];
        else if (!strcmp(argv[i], "--timeout") && i + 1 < argc) timeoutSec = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--token") && i + 1 < argc) token = argv[++i];
        else if (!strcmp(argv[i], "--require-encrypt") && i + 1 < argc) requireEncrypt = atoi(argv[++i]) != 0;
        else { std::fprintf(stderr, "usage: %s --hash <16-hex> [--name NAME] [--timeout SEC] [--token STR] [--require-encrypt 0|1]\n", argv[0]); return 2; }
    }
    if (hash.empty()) { std::fprintf(stderr, "need --hash\n"); return 2; }

    g_service_uuid = build_service_uuid(hash);
    g_local_name = name;
    g_timeout_sec = timeoutSec;
    g_expected_token = token;
    g_require_encrypt = requireEncrypt;

    GError* err = nullptr;
    std::cerr << "[STEP] get system bus\n";
    GDBusConnection* conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &err);
    if (!conn) { std::cerr << "[ERR] system bus: " << (err ? err->message : "unknown") << "\n"; if (err) g_error_free(err); return 2; }

    std::string adapterPath = get_adapter_path(conn);
    std::cout << "[BLE] Adapter: " << adapterPath << "\n";

    std::cerr << "[STEP] set Alias\n";
    g_dbus_connection_call_sync(conn, "org.bluez", adapterPath.c_str(),
        "org.freedesktop.DBus.Properties", "Set",
        g_variant_new("(ssv)", "org.bluez.Adapter1", "Alias", g_variant_new_string(g_local_name.c_str())),
        nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);

    std::cerr << "[STEP] parse XML\n";
    GDBusNodeInfo* service_node = g_dbus_node_info_new_for_xml(SERVICE_IF_XML, &err);
    if (!service_node) { std::cerr << "[ERR] service xml: " << err->message << "\n"; g_error_free(err); return 2; }
    GDBusNodeInfo* char_node = g_dbus_node_info_new_for_xml(CHAR_IF_XML, &err);
    if (!char_node) { std::cerr << "[ERR] char xml: " << err->message << "\n"; g_error_free(err); return 2; }
    GDBusNodeInfo* adv_node = g_dbus_node_info_new_for_xml(ADV_IF_XML, &err);
    if (!adv_node) { std::cerr << "[ERR] adv xml: " << err->message << "\n"; g_error_free(err); return 2; }
    GDBusNodeInfo* objmgr_node = g_dbus_node_info_new_for_xml(OBJMGR_IF_XML, &err);
    if (!objmgr_node) { std::cerr << "[ERR] objmgr xml: " << err->message << "\n"; g_error_free(err); return 2; }

    std::cerr << "[STEP] register objects\n";
    guint reg_service = g_dbus_connection_register_object(conn, SERVICE_PATH, service_node->interfaces[0], &SERVICE_VTABLE, nullptr, nullptr, &err);
    if (!reg_service) { std::cerr << "[ERR] register service: " << (err ? err->message : "unknown") << "\n"; if (err) g_error_free(err); return 2; }
    guint reg_char = g_dbus_connection_register_object(conn, CHAR_PATH, char_node->interfaces[0], &CHAR_VTABLE, nullptr, nullptr, &err);
    if (!reg_char) { std::cerr << "[ERR] register char: " << (err ? err->message : "unknown") << "\n"; if (err) g_error_free(err); return 2; }
    guint reg_adv = g_dbus_connection_register_object(conn, ADV_PATH, adv_node->interfaces[0], &ADV_VTABLE, nullptr, nullptr, &err);
    if (!reg_adv) { std::cerr << "[ERR] register adv: " << (err ? err->message : "unknown") << "\n"; if (err) g_error_free(err); return 2; }
    guint reg_objmgr = g_dbus_connection_register_object(
        conn, APP_PATH, objmgr_node->interfaces[0], &OBJMGR_VTABLE, nullptr, nullptr, &err);
    if (!reg_objmgr) { std::cerr << "[ERR] register objmgr: " << (err ? err->message : "unknown") << "\n"; if (err) g_error_free(err); return 2; }

    std::cerr << "[STEP] RegisterApplication (async)\n";

    GVariant* options = g_variant_new_array(G_VARIANT_TYPE("{sv}"), nullptr, 0); // 빈 a{sv}
    GMainLoop* loop = g_main_loop_new(nullptr, FALSE);
    bool app_registered = false;

    g_dbus_connection_call(
        conn, "org.bluez", adapterPath.c_str(),
        "org.bluez.GattManager1", "RegisterApplication",
        g_variant_new("(o@a{sv})", APP_PATH, options),   // @ 필수
        nullptr, G_DBUS_CALL_FLAGS_NONE, 5000, nullptr,
        [](GObject* source, GAsyncResult* res, gpointer user_data) {
            GError* e = nullptr;
            GVariant* r = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source), res, &e);
            bool* ok = static_cast<bool*>(user_data);
            if (!r) {
                std::cerr << "[ERR] RegisterApplication: " << (e ? e->message : "unknown") << "\n";
                if (e) g_error_free(e);
                *ok = false;
            }
            else {
                std::cerr << "[STEP] RegisterApplication ok\n";
                g_variant_unref(r);
                *ok = true;
            }
        },
        &app_registered
    );

    while (!app_registered) g_main_context_iteration(nullptr, TRUE);

    std::cerr << "[STEP] RegisterAdvertisement\n";
    ret = g_dbus_connection_call_sync(conn, "org.bluez", adapterPath.c_str(),
        "org.bluez.LEAdvertisingManager1", "RegisterAdvertisement",
        g_variant_new("(o@a{sv})", ADV_PATH, g_variant_new("a{sv}", nullptr)),
        nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &err);
    if (!ret) { std::cerr << "[ERR] RegisterAdvertisement: " << (err ? err->message : "unknown") << "\n"; if (err) g_error_free(err); return 2; }
    g_variant_unref(ret);

    std::cout << "[BLE] Advertising service " << g_service_uuid
        << " name=" << g_local_name
        << " expect=\"" << g_expected_token << "\""
        << " encrypt=" << (g_require_encrypt ? "on" : "off")
        << " timeout=" << g_timeout_sec << "s\n";

    std::cerr << "[STEP] loop start\n";
    int elapsed_ms = 0;
    while (!g_done && elapsed_ms < g_timeout_sec * 1000) {
        g_main_context_iteration(nullptr, FALSE);
        g_usleep(200 * 1000);
        elapsed_ms += 200;
    }

    std::cerr << "[STEP] cleanup\n";
    g_dbus_connection_call_sync(conn, "org.bluez", adapterPath.c_str(),
        "org.bluez.LEAdvertisingManager1", "UnregisterAdvertisement",
        g_variant_new("(o)", ADV_PATH), nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);

    g_dbus_connection_unregister_object(conn, reg_adv);
    g_dbus_connection_unregister_object(conn, reg_char);
    g_dbus_connection_unregister_object(conn, reg_service);
    g_dbus_connection_unregister_object(conn, reg_objmgr);

    g_dbus_node_info_unref(adv_node);
    g_dbus_node_info_unref(char_node);
    g_dbus_node_info_unref(service_node);
    g_dbus_node_info_unref(objmgr_node);

    return (g_done && g_ok) ? 0 : 1;
}
