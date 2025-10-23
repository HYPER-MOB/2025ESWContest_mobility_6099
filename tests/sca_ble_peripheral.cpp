// sca_ble_peripheral.cpp
// Build:
//   g++ -std=c++17 sca_ble_peripheral.cpp -o sca_ble_peripheral `pkg-config --cflags --libs glib-2.0 gio-2.0`
// Usage:
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

// ===== Config =====
static const char* BASE_UUID_PREFIX = "12345678-0000-1000-8000-"; // 16 hex가 붙어 128-bit 서비스 UUID 완성
static const char* ACCESS_CHAR_UUID = "c0de0001-0000-1000-8000-000000000001"; // write-only characteristic

// ===== Globals (런타임 값) =====
static std::string g_service_uuid;
static std::string g_local_name = "SCA-CAR";
static std::string g_expected_token = "ACCESS";
static int g_timeout_sec = 30;
static bool g_require_encrypt = false;

static bool g_done = false; // write 발생 여부
static bool g_ok = false; // 토큰 일치 여부

// DBus object paths
static const char* APP_PATH = "/com/sca/app";
static const char* SERVICE_PATH = "/com/sca/app/service0";
static const char* CHAR_PATH = "/com/sca/app/service0/char0";
static const char* ADV_PATH = "/com/sca/app/adv0";

// ===== XML(Introspection) =====
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

// LEAdvertisement1 은 RegisterAdvertisement 시 BlueZ가 인트로스펙션/프로퍼티를 읽어갑니다.
static const char* ADV_IF_XML = R"XML(
<node>
  <interface name="org.bluez.LEAdvertisement1">
    <method name="Release"/>
    <property name="Type"          type="s"  access="read"/>
    <property name="ServiceUUIDs"  type="as" access="read"/>
    <property name="LocalName"     type="s"  access="read"/>
    <property name="Discoverable"  type="b"  access="read"/>
    <!-- 필요 시 Appearance, ManufacturerData, SolicitUUIDs 등 확장 가능 -->
  </interface>
</node>
)XML";

// ===== Utils =====
static std::string build_service_uuid(const std::string& hash16) {
    if (hash16.size() != 16) {
        std::cerr << "[ERR] --hash must be 16 hex chars\n";
        std::exit(2);
    }
    std::string up = hash16;
    for (auto& c : up) c = (char)std::toupper((unsigned char)c);
    return std::string(BASE_UUID_PREFIX) + up;
}

static std::string get_adapter_path(GDBusConnection* conn) {
    GError* err = nullptr;
    GVariant* ret = g_dbus_connection_call_sync(
        conn, "org.bluez", "/", "org.freedesktop.DBus.ObjectManager", "GetManagedObjects",
        nullptr, G_VARIANT_TYPE("(a{oa{sa{sv}}})"),
        G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &err);
    if (!ret) {
        std::cerr << "[ERR] GetManagedObjects: " << (err ? err->message : "unknown") << "\n";
        if (err) g_error_free(err);
        std::exit(2);
    }

    GVariantIter* i = nullptr;
    const gchar* objpath;
    GVariant* ifaces = nullptr;
    g_variant_get(ret, "(a{oa{sa{sv}}})", &i);

    std::string adapterPath;
    while (g_variant_iter_loop(i, "{&oa{sa{sv}}}", &objpath, &ifaces)) {
        GVariantIter* ii = nullptr;
        const gchar* ifname;
        GVariant* props;
        g_variant_get(ifaces, "a{sa{sv}}", &ii);
        while (g_variant_iter_loop(ii, "{&sa{sv}}", &ifname, &props)) {
            if (std::string(ifname) == "org.bluez.Adapter1") {
                adapterPath = objpath;
            }
        }
        g_variant_iter_free(ii);
    }
    g_variant_iter_free(i);
    g_variant_unref(ret);

    if (adapterPath.empty()) {
        std::cerr << "[ERR] No Adapter1 found\n";
        std::exit(2);
    }
    return adapterPath;
}

// ===== Property Getters (정확한 시그니처) =====
static GVariant* service_get(GDBusConnection*, const gchar* /*sender*/,
    const gchar* /*object_path*/, const gchar* /*interface_name*/,
    const gchar* property_name, GError** /*error*/, gpointer /*user_data*/) {

    if (g_strcmp0(property_name, "UUID") == 0)
        return g_variant_new_string(g_service_uuid.c_str());
    if (g_strcmp0(property_name, "Primary") == 0)
        return g_variant_new_boolean(TRUE);
    if (g_strcmp0(property_name, "Includes") == 0)
        return g_variant_new("ao", nullptr);
    if (g_strcmp0(property_name, "Device") == 0)
        return g_variant_new_object_path("/");
    return nullptr;
}

static GVariant* char_get(GDBusConnection*, const gchar* /*sender*/,
    const gchar* /*object_path*/, const gchar* /*interface_name*/,
    const gchar* property_name, GError** /*error*/, gpointer /*user_data*/) {

    if (g_strcmp0(property_name, "UUID") == 0)
        return g_variant_new_string(ACCESS_CHAR_UUID);
    if (g_strcmp0(property_name, "Service") == 0)
        return g_variant_new_object_path(SERVICE_PATH);
    if (g_strcmp0(property_name, "Value") == 0)
        return g_variant_new_from_data(G_VARIANT_TYPE("ay"), "", 0, TRUE, nullptr, nullptr);
    if (g_strcmp0(property_name, "Flags") == 0) {
        if (g_require_encrypt) {
            const gchar* flags[] = { "write", "encrypt-write", nullptr };
            return g_variant_new_strv(flags, -1);
        }
        else {
            const gchar* flags[] = { "write", nullptr };
            return g_variant_new_strv(flags, -1);
        }
    }
    return nullptr;
}

static GVariant* adv_get(GDBusConnection*, const gchar* /*sender*/,
    const gchar* /*object_path*/, const gchar* /*interface_name*/,
    const gchar* property_name, GError** /*error*/, gpointer /*user_data*/) {

    if (g_strcmp0(property_name, "Type") == 0)
        return g_variant_new_string("peripheral");
    if (g_strcmp0(property_name, "LocalName") == 0)
        return g_variant_new_string(g_local_name.c_str());
    if (g_strcmp0(property_name, "Discoverable") == 0)
        return g_variant_new_boolean(TRUE);
    if (g_strcmp0(property_name, "ServiceUUIDs") == 0) {
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE("as"));
        g_variant_builder_add(&b, "s", g_service_uuid.c_str());
        return g_variant_builder_end(&b);
    }
    return nullptr;
}

// ===== Method Handlers =====
static void char_method_call(GDBusConnection* /*conn*/, const gchar* /*sender*/,
    const gchar* /*obj_path*/, const gchar* /*iface*/, const gchar* method,
    GVariant* params, GDBusMethodInvocation* invoc, gpointer /*user_data*/) {

    if (g_strcmp0(method, "WriteValue") == 0) {
        GVariant* value = nullptr, * options = nullptr;
        // (@ay@a{sv}) : value(ay), options(a{sv})
        g_variant_get(params, "(@ay@a{sv})", &value, &options);

        gsize n = 0;
        const guchar* bytes = (const guchar*)g_variant_get_fixed_array(value, &n, sizeof(guchar));
        std::string data((const char*)bytes, (const char*)bytes + n);

        std::cout << "[BLE] WriteValue len=" << n << " data=\"";
        for (gsize i = 0; i < n; ++i) {
            unsigned char c = bytes[i];
            if (c >= 32 && c < 127) std::cout << (char)c;
            else std::cout << "\\x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)c << std::dec;
        }
        std::cout << "\"\n";

        g_done = true;
        if (data == g_expected_token) {
            g_ok = true;
            g_dbus_method_invocation_return_value(invoc, nullptr);
        }
        else {
            g_ok = false;
            g_dbus_method_invocation_return_dbus_error(
                invoc, "com.sca.Error.InvalidPayload", "Invalid payload");
        }

        if (options) g_variant_unref(options);
        if (value) g_variant_unref(value);
        return;
    }
    else if (g_strcmp0(method, "ReadValue") == 0) {
        // 읽을 값은 없음. 빈 배열 반환
        GVariant* options = nullptr;
        g_variant_get(params, "(@a{sv})", &options);
        if (options) g_variant_unref(options);

        GVariant* empty = g_variant_new_from_data(G_VARIANT_TYPE("ay"), "", 0, TRUE, nullptr, nullptr);
        g_dbus_method_invocation_return_value(invoc, g_variant_new_tuple(&empty, 1));
        return;
    }

    g_dbus_method_invocation_return_dbus_error(
        invoc, "org.freedesktop.DBus.Error.UnknownMethod", "Unknown method");
}

static void adv_method_call(GDBusConnection*, const gchar*, const gchar*,
    const gchar*, const gchar* method, GVariant* /*params*/,
    GDBusMethodInvocation* invoc, gpointer /*user_data*/) {
    if (g_strcmp0(method, "Release") == 0) {
        g_dbus_method_invocation_return_value(invoc, nullptr);
        return;
    }
    g_dbus_method_invocation_return_dbus_error(
        invoc, "org.freedesktop.DBus.Error.UnknownMethod", "Unknown method");
}

// ===== VTables (정적 수명으로 유지) =====
static const GDBusInterfaceVTable SERVICE_VTABLE = {
    /*method_call*/ nullptr,
    /*get_property*/ service_get,
    /*set_property*/ nullptr
};
static const GDBusInterfaceVTable CHAR_VTABLE = {
    /*method_call*/ char_method_call,
    /*get_property*/ char_get,
    /*set_property*/ nullptr
};
static const GDBusInterfaceVTable ADV_VTABLE = {
    /*method_call*/ adv_method_call,
    /*get_property*/ adv_get,
    /*set_property*/ nullptr
};

int main(int argc, char** argv) {
    // ---- Parse args ----
    std::string hash, name = "SCA-CAR";
    int timeoutSec = 30;
    std::string token = "ACCESS";
    bool requireEncrypt = false;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--hash") && i + 1 < argc) hash = argv[++i];
        else if (!strcmp(argv[i], "--name") && i + 1 < argc) name = argv[++i];
        else if (!strcmp(argv[i], "--timeout") && i + 1 < argc) timeoutSec = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--token") && i + 1 < argc) token = argv[++i];
        else if (!strcmp(argv[i], "--require-encrypt") && i + 1 < argc) requireEncrypt = atoi(argv[++i]) != 0;
        else {
            std::fprintf(stderr, "usage: %s --hash <16-hex> [--name NAME] [--timeout SEC] [--token STR] [--require-encrypt 0|1]\n", argv[0]);
            return 2;
        }
    }
    if (hash.empty()) { std::fprintf(stderr, "need --hash\n"); return 2; }

    g_service_uuid = build_service_uuid(hash);
    g_local_name = name;
    g_timeout_sec = timeoutSec;
    g_expected_token = token;
    g_require_encrypt = requireEncrypt;

    // ---- System bus ----
    GError* err = nullptr;
    GDBusConnection* conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &err);
    if (!conn) {
        std::cerr << "[ERR] system bus: " << (err ? err->message : "unknown") << "\n";
        if (err) g_error_free(err);
        return 2;
    }

    // ---- Adapter ----
    std::string adapterPath = get_adapter_path(conn);
    std::cout << "[BLE] Adapter: " << adapterPath << "\n";

    // Alias 변경(표시 이름)
    g_dbus_connection_call_sync(conn, "org.bluez", adapterPath.c_str(),
        "org.freedesktop.DBus.Properties", "Set",
        g_variant_new("(ssv)", "org.bluez.Adapter1", "Alias",
            g_variant_new_string(g_local_name.c_str())),
        nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);

    // ---- Parse interface XMLs (수명 유지 → 마지막에 unref) ----
    GDBusNodeInfo* service_node = g_dbus_node_info_new_for_xml(SERVICE_IF_XML, &err);
    if (!service_node) { std::cerr << "[ERR] service xml: " << err->message << "\n"; g_error_free(err); return 2; }
    GDBusNodeInfo* char_node = g_dbus_node_info_new_for_xml(CHAR_IF_XML, &err);
    if (!char_node) { std::cerr << "[ERR] char xml: " << err->message << "\n"; g_error_free(err); return 2; }
    GDBusNodeInfo* adv_node = g_dbus_node_info_new_for_xml(ADV_IF_XML, &err);
    if (!adv_node) { std::cerr << "[ERR] adv xml: " << err->message << "\n"; g_error_free(err); return 2; }

    // ---- Export service/char/adv objects ----
    guint reg_service = g_dbus_connection_register_object(
        conn, SERVICE_PATH, service_node->interfaces[0],
        &SERVICE_VTABLE, nullptr, nullptr, &err);
    if (!reg_service) { std::cerr << "[ERR] register service: " << (err ? err->message : "unknown") << "\n"; if (err) g_error_free(err); return 2; }

    guint reg_char = g_dbus_connection_register_object(
        conn, CHAR_PATH, char_node->interfaces[0],
        &CHAR_VTABLE, nullptr, nullptr, &err);
    if (!reg_char) { std::cerr << "[ERR] register char: " << (err ? err->message : "unknown") << "\n"; if (err) g_error_free(err); return 2; }

    guint reg_adv = g_dbus_connection_register_object(
        conn, ADV_PATH, adv_node->interfaces[0],
        &ADV_VTABLE, nullptr, nullptr, &err);
    if (!reg_adv) { std::cerr << "[ERR] register adv: " << (err ? err->message : "unknown") << "\n"; if (err) g_error_free(err); return 2; }

    // ---- Register GATT Application (어댑터 경로!) ----
    GVariant* ret = g_dbus_connection_call_sync(
        conn, "org.bluez", adapterPath.c_str(),
        "org.bluez.GattManager1", "RegisterApplication",
        g_variant_new("(oa{sv})", APP_PATH, g_variant_new("a{sv}", nullptr)),
        nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &err);
    if (!ret) { std::cerr << "[ERR] RegisterApplication: " << (err ? err->message : "unknown") << "\n"; if (err) g_error_free(err); return 2; }
    g_variant_unref(ret);

    // ---- Register Advertisement (어댑터 경로!) ----
    ret = g_dbus_connection_call_sync(
        conn, "org.bluez", adapterPath.c_str(),
        "org.bluez.LEAdvertisingManager1", "RegisterAdvertisement",
        g_variant_new("(oa{sv})", ADV_PATH, g_variant_new("a{sv}", nullptr)),
        nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &err);
    if (!ret) { std::cerr << "[ERR] RegisterAdvertisement: " << (err ? err->message : "unknown") << "\n"; if (err) g_error_free(err); return 2; }
    g_variant_unref(ret);

    std::cout << "[BLE] Advertising service " << g_service_uuid
        << " name=" << g_local_name
        << " expect=\"" << g_expected_token << "\""
        << " encrypt=" << (g_require_encrypt ? "on" : "off")
        << " timeout=" << g_timeout_sec << "s\n";

    // ---- Main wait loop ----
    int elapsed_ms = 0;
    while (!g_done && elapsed_ms < g_timeout_sec * 1000) {
        g_main_context_iteration(nullptr, FALSE);
        g_usleep(200 * 1000);
        elapsed_ms += 200;
    }

    // ---- Cleanup ----
    g_dbus_connection_call_sync(
        conn, "org.bluez", adapterPath.c_str(),
        "org.bluez.LEAdvertisingManager1", "UnregisterAdvertisement",
        g_variant_new("(o)", ADV_PATH),
        nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);

    g_dbus_connection_unregister_object(conn, reg_adv);
    g_dbus_connection_unregister_object(conn, reg_char);
    g_dbus_connection_unregister_object(conn, reg_service);

    g_dbus_node_info_unref(adv_node);
    g_dbus_node_info_unref(char_node);
    g_dbus_node_info_unref(service_node);

    return (g_done && g_ok) ? 0 : 1;
}
