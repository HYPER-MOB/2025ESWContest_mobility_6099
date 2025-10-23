// sca_ble_peripheral.cpp
// Build:
//   g++ -std=c++17 sca_ble_peripheral.cpp -o sca_ble_peripheral `pkg-config --cflags --libs glib-2.0 gio-2.0`
// Usage:
//   sudo ./sca_ble_peripheral --hash A1B2C3D4E5F60708 --name CAR123 --timeout 30 --token ACCESS --require-encrypt 0

#include <gio/gio.h>
#include <glib.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <iomanip>

// ---- Config / Defaults ----
static const char* BASE_UUID_PREFIX = "12345678-0000-1000-8000-";
static const char* ACCESS_CHAR_UUID = "c0de0001-0000-1000-8000-000000000001";

static std::string g_service_uuid;
static std::string g_local_name = "SCA-CAR";
static std::string g_expected_token = "ACCESS";
static int  g_timeout_sec = 30;
static bool g_require_encrypt = false;

static bool g_done = false; // whether any write happened
static bool g_ok = false; // whether token matched

static const char* APP_PATH = "/com/sca/app";
static const char* SERVICE_PATH = "/com/sca/app/service0";
static const char* CHAR_PATH = "/com/sca/app/service0/char0";

// ---- Utils ----
static std::string to_upper(std::string s) {
    for (auto& c : s) c = (char)toupper((unsigned char)c);
    return s;
}
static std::string build_service_uuid(const std::string& hash16) {
    if (hash16.size() != 16) {
        std::cerr << "[ERR] --hash must be 16 hex chars (8 bytes)\n";
        std::exit(2);
    }
    return std::string(BASE_UUID_PREFIX) + to_upper(hash16);
}

// Find first Adapter (hciX)
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
    GVariant* ifaces;
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
        std::cerr << "[ERR] No org.bluez.Adapter1 found\n";
        std::exit(2);
    }
    return adapterPath;
}

// ---- Service / Char Interface XML ----
static const char* SERVICE_IF_XML = R"XML(
<node>
  <interface name="org.bluez.GattService1">
    <property type="s" name="UUID" access="read"/>
    <property type="o" name="Device" access="read"/>
    <property type="b" name="Primary" access="read"/>
    <property type="ao" name="Includes" access="read"/>
  </interface>
</node>
)XML";

static const char* CHAR_IF_XML = R"XML(
<node>
  <interface name="org.bluez.GattCharacteristic1">
    <method name="ReadValue">
      <arg type="ay" name="Value" direction="out"/>
      <arg type="a{sv}" name="Options" direction="in"/>
    </method>
    <method name="WriteValue">
      <arg type="ay" name="Value" direction="in"/>
      <arg type="a{sv}" name="Options" direction="in"/>
    </method>
    <property type="s" name="UUID" access="read"/>
    <property type="o" name="Service" access="read"/>
    <property type="ay" name="Value" access="read"/>
    <property type="as" name="Flags" access="read"/>
  </interface>
</node>
)XML";

// ---- Property Getters ----
static GVariant* service_get(GDBusConnection*, const gchar*, const gchar*,
    const gchar* prop, GError**, gpointer) {
    if (std::string(prop) == "UUID")     return g_variant_new_string(g_service_uuid.c_str());
    if (std::string(prop) == "Primary")  return g_variant_new_boolean(TRUE);
    if (std::string(prop) == "Includes") return g_variant_new("ao", nullptr);
    if (std::string(prop) == "Device")   return g_variant_new_object_path("/");
    return nullptr;
}
static GVariant* char_get(GDBusConnection*, const gchar*, const gchar*,
    const gchar* prop, GError**, gpointer) {
    if (std::string(prop) == "UUID")    return g_variant_new_string(ACCESS_CHAR_UUID);
    if (std::string(prop) == "Service") return g_variant_new_object_path(SERVICE_PATH);
    if (std::string(prop) == "Value")   return g_variant_new_from_data(G_VARIANT_TYPE("ay"), "", 0, TRUE, nullptr, nullptr);
    if (std::string(prop) == "Flags") {
        if (g_require_encrypt) {
            const gchar* flags[] = { "write", "encrypt-write", nullptr };
            return g_variant_new_strv(flags, -1);
        }
        else {
            const gchar* flags[] = { "write", /*"write-without-response",*/ nullptr };
            return g_variant_new_strv(flags, -1);
        }
    }
    return nullptr;
}

static const GDBusInterfaceInfo* parse_xml(const char* xml) {
    GError* err = nullptr;
    GDBusNodeInfo* info = g_dbus_node_info_new_for_xml(xml, &err);
    if (!info) {
        std::cerr << "[ERR] XML parse: " << err->message << "\n";
        g_error_free(err);
        std::exit(2);
    }
    const GDBusInterfaceInfo* iface = info->interfaces[0];
    // NOTE: info는 여기서만 임시로 쓰고 버림. iface 포인터는 수명 보장이 어려우므로
    // 안전하게 복사해서 쓰는 게 정석이지만, 간단화를 위해 여기선 즉시 register 시 사용.
    return iface;
}

// ---- WriteValue handler ----
static void on_method_call(GDBusConnection* conn, const gchar* sender, const gchar* obj_path,
    const gchar* iface, const gchar* method, GVariant* params,
    GDBusMethodInvocation* invoc, gpointer /*user_data*/) {
    if (std::string(iface) == "org.bluez.GattCharacteristic1" &&
        std::string(method) == "WriteValue") {

        GVariant* value = nullptr;
        GVariant* options = nullptr; // a{sv}
        g_variant_get(params, "(@ay@a{sv})", &value, &options);

        gsize n = 0;
        gconstpointer bytes = g_variant_get_fixed_array(value, &n, sizeof(guchar));
        std::string data((const char*)bytes, (const char*)bytes + n);

        std::cout << "[BLE] WriteValue len=" << n << " data=\"";
        for (size_t i = 0; i < n; ++i) {
            unsigned char c = ((const unsigned char*)bytes)[i];
            if (c >= 32 && c < 127) std::cout << (char)c;
            else std::cout << "\\x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)c << std::dec;
        }
        std::cout << "\"\n";

        // (선택) 옵션에서 device 경로를 뽑아 특정 디바이스 필터링 가능
        // const gchar* dev_path = nullptr;
        // if (options) {
        //     GVariantIter* opti = nullptr;
        //     const gchar* k; GVariant* v;
        //     g_variant_get(options, "a{sv}", &opti);
        //     while (g_variant_iter_loop(opti, "{&sv}", &k, &v)) {
        //         if (g_str_equal(k, "device")) dev_path = g_variant_get_string(v, nullptr);
        //     }
        //     g_variant_iter_free(opti);
        // }

        g_done = true;
        if (data == g_expected_token) {
            g_ok = true;
            g_dbus_method_invocation_return_value(invoc, nullptr); // success
        }
        else {
            g_ok = false;
            g_dbus_method_invocation_return_dbus_error(
                invoc,
                "com.sca.Error.InvalidPayload",
                "Invalid payload for ACCESS");
        }

        if (options) g_variant_unref(options);
        if (value) g_variant_unref(value);
        return;
    }

    g_dbus_method_invocation_return_dbus_error(
        invoc, "org.freedesktop.DBus.Error.UnknownMethod", "Unknown method");
}

static const GDBusInterfaceVTable char_vtable = {
    on_method_call, nullptr, nullptr
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
            std::printf("usage: %s --hash <16-hex> [--name NAME] [--timeout SEC] [--token STR] [--require-encrypt 0|1]\n", argv[0]);
            return 2;
        }
    }
    if (hash.empty()) { std::puts("need --hash"); return 2; }

    g_service_uuid = build_service_uuid(hash);
    g_local_name = name;
    g_timeout_sec = timeoutSec;
    g_expected_token = token;
    g_require_encrypt = requireEncrypt;

    // ---- System bus ----
    GError* err = nullptr;
    GDBusConnection* conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &err);
    if (!conn) {
        std::cerr << "[ERR] get system bus: " << (err ? err->message : "unknown") << "\n";
        if (err) g_error_free(err);
        return 2;
    }

    // ---- Adapter ----
    std::string adapterPath = get_adapter_path(conn);
    std::cout << "[BLE] Adapter: " << adapterPath << "\n";

    // Set local alias
    g_dbus_connection_call_sync(conn, "org.bluez", adapterPath.c_str(),
        "org.freedesktop.DBus.Properties", "Set",
        g_variant_new("(ssv)", "org.bluez.Adapter1", "Alias",
            g_variant_new_string(g_local_name.c_str())),
        nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);

    // ---- Register Service / Char objects ----
    guint reg_service = g_dbus_connection_register_object(
        conn, SERVICE_PATH, parse_xml(SERVICE_IF_XML),
        &(GDBusInterfaceVTable){nullptr, nullptr, nullptr},
        nullptr,
        (GDBusInterfaceGetPropertyFunc)service_get,
        nullptr, & err);
    if (!reg_service) {
        std::cerr << "[ERR] register service: " << (err ? err->message : "unknown") << "\n";
        if (err) g_error_free(err);
        return 2;
    }

    guint reg_char = g_dbus_connection_register_object(
        conn, CHAR_PATH, parse_xml(CHAR_IF_XML),
        &char_vtable, nullptr,
        (GDBusInterfaceGetPropertyFunc)char_get,
        nullptr, &err);
    if (!reg_char) {
        std::cerr << "[ERR] register char: " << (err ? err->message : "unknown") << "\n";
        if (err) g_error_free(err);
        return 2;
    }

    // ---- Register GATT Application ----
    GVariant* app_dict = g_variant_new("a{sv}", nullptr);
    GVariant* ret = g_dbus_connection_call_sync(
        conn, "org.bluez", (adapterPath + "/org/bluez").c_str(),
        "org.bluez.GattManager1", "RegisterApplication",
        g_variant_new("(oa{sv})", APP_PATH, app_dict),
        nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &err);
    if (!ret) {
        std::cerr << "[ERR] RegisterApplication: " << (err ? err->message : "unknown") << "\n";
        if (err) g_error_free(err);
        return 2;
    }
    g_variant_unref(ret);

    // ---- LE Advertising (simple) ----
    // NOTE: 엄밀히는 org.bluez.LEAdvertisement1 오브젝트를 export해야 하지만,
    // 기존 코드 방식 유지(옵션 딕셔너리 전달). 배포판/버전에 따라 실제 구현이 다를 수 있음.
    GVariantBuilder svcUuids; g_variant_builder_init(&svcUuids, G_VARIANT_TYPE("as"));
    g_variant_builder_add(&svcUuids, "s", g_service_uuid.c_str());

    GVariantBuilder props; g_variant_builder_init(&props, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&props, "{sv}", "Type", g_variant_new_string("peripheral"));
    g_variant_builder_add(&props, "{sv}", "ServiceUUIDs", g_variant_builder_end(&svcUuids));
    g_variant_builder_add(&props, "{sv}", "LocalName", g_variant_new_string(g_local_name.c_str()));
    g_variant_builder_add(&props, "{sv}", "Discoverable", g_variant_new_boolean(TRUE));

    const char* ADV_PATH = "/com/sca/app/adv0";
    ret = g_dbus_connection_call_sync(
        conn, "org.bluez", (adapterPath + "/org/bluez").c_str(),
        "org.bluez.LEAdvertisingManager1", "RegisterAdvertisement",
        g_variant_new("(oa{sv})", ADV_PATH, g_variant_builder_end(&props)),
        nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &err);
    if (!ret) {
        std::cerr << "[ERR] RegisterAdvertisement: " << (err ? err->message : "unknown") << "\n";
        if (err) g_error_free(err);
        return 2;
    }
    g_variant_unref(ret);

    std::cout << "[BLE] Advertising service " << g_service_uuid
        << " name=" << g_local_name
        << " expect=\"" << g_expected_token << "\""
        << " encrypt=" << (g_require_encrypt ? "on" : "off")
        << " timeout=" << g_timeout_sec << "s\n";

    // ---- Main loop (wait for write or timeout) ----
    int elapsed_ms = 0;
    while (!g_done && elapsed_ms < g_timeout_sec * 1000) {
        g_main_context_iteration(nullptr, FALSE);
        g_usleep(200 * 1000);
        elapsed_ms += 200;
    }

    // ---- Cleanup ----
    g_dbus_connection_call_sync(
        conn, "org.bluez", (adapterPath + "/org/bluez").c_str(),
        "org.bluez.LEAdvertisingManager1", "UnregisterAdvertisement",
        g_variant_new("(o)", ADV_PATH), nullptr,
        G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);

    g_dbus_connection_unregister_object(conn, reg_char);
    g_dbus_connection_unregister_object(conn, reg_service);

    return (g_done && g_ok) ? 0 : 1;
}
