// sca_ble_peripheral.cpp
// Build:
//   g++ -std=c++17 sca_ble_peripheral.cpp -o sca_ble_peripheral `pkg-config --cflags --libs glib-2.0 gio-2.0`
// Run (example):
//   sudo ./sca_ble_peripheral --hash A1B2C3D4E5F60708 --name SCA-CAR --timeout 30 --token ACCESS --require-encrypt 0

#include <gio/gio.h>
#include <glib.h>
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

static bool g_done = false; // write happened
static bool g_ok = false; // token matched

static const char* APP_PATH = "/com/sca/app";
static const char* SERVICE_PATH = "/com/sca/app/service0";
static const char* CHAR_PATH = "/com/sca/app/service0/char0";
static const char* ADV_PATH = "/com/sca/app/adv0";

// -------- Utils --------
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
        nullptr, G_VARIANT_TYPE("(a{oa{sa{sv}}})"), G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &err);
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

// -------- GATT Service/Characteristic Introspection XML --------
static const char* SERVICE_IF_XML = R"XML(
<node>
  <interface name="org.bluez.GattService1">
    <property type="s"  name="UUID" access="read"/>
    <property type="o"  name="Device" access="read"/>
    <property type="b"  name="Primary" access="read"/>
    <property type="ao" name="Includes" access="read"/>
  </interface>
</node>
)XML";

static const char* CHAR_IF_XML = R"XML(
<node>
  <interface name="org.bluez.GattCharacteristic1">
    <method name="ReadValue">
      <arg type="ay"   name="Value"   direction="out"/>
      <arg type="a{sv}" name="Options" direction="in"/>
    </method>
    <method name="WriteValue">
      <arg type="ay"   name="Value"   direction="in"/>
      <arg type="a{sv}" name="Options" direction="in"/>
    </method>
    <property type="s"  name="UUID"    access="read"/>
    <property type="o"  name="Service" access="read"/>
    <property type="ay" name="Value"   access="read"/>
    <property type="as" name="Flags"   access="read"/>
  </interface>
</node>
)XML";

// -------- LEAdvertisement1 Introspection XML --------
static const char* ADV_IF_XML = R"XML(
<node>
  <interface name="org.bluez.LEAdvertisement1">
    <method name="Release"/>
    <property type="s"  name="Type" access="read"/>
    <property type="as" name="ServiceUUIDs" access="read"/>
    <property type="s"  name="LocalName" access="read"/>
    <property type="b"  name="Discoverable" access="read"/>
  </interface>
</node>
)XML";

// -------- Property Getters (correct signatures) --------
static GVariant* service_get(GDBusConnection*, const gchar* /*sender*/,
    const gchar* /*obj_path*/, const gchar* interface_name,
    const gchar* property_name, GError**, gpointer) {
    if (std::string(interface_name) != "org.bluez.GattService1") return nullptr;
    if (std::string(property_name) == "UUID")      return g_variant_new_string(g_service_uuid.c_str());
    if (std::string(property_name) == "Primary")   return g_variant_new_boolean(TRUE);
    if (std::string(property_name) == "Includes")  return g_variant_new("ao", nullptr);
    if (std::string(property_name) == "Device")    return g_variant_new_object_path("/");
    return nullptr;
}

static GVariant* char_get(GDBusConnection*, const gchar* /*sender*/,
    const gchar* /*obj_path*/, const gchar* interface_name,
    const gchar* property_name, GError**, gpointer) {
    if (std::string(interface_name) != "org.bluez.GattCharacteristic1") return nullptr;
    if (std::string(property_name) == "UUID")     return g_variant_new_string(ACCESS_CHAR_UUID);
    if (std::string(property_name) == "Service")  return g_variant_new_object_path(SERVICE_PATH);
    if (std::string(property_name) == "Value")    return g_variant_new_from_data(G_VARIANT_TYPE("ay"), "", 0, TRUE, nullptr, nullptr);
    if (std::string(property_name) == "Flags") {
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

// Advertisement properties
static GVariant* adv_get(GDBusConnection*, const gchar* /*sender*/,
    const gchar* /*obj_path*/, const gchar* interface_name,
    const gchar* property_name, GError**, gpointer) {
    if (std::string(interface_name) != "org.bluez.LEAdvertisement1") return nullptr;

    if (std::string(property_name) == "Type") {
        return g_variant_new_string("peripheral");
    }
    if (std::string(property_name) == "ServiceUUIDs") {
        GVariantBuilder b; g_variant_builder_init(&b, G_VARIANT_TYPE("as"));
        g_variant_builder_add(&b, "s", g_service_uuid.c_str());
        return g_variant_builder_end(&b);
    }
    if (std::string(property_name) == "LocalName") {
        return g_variant_new_string(g_local_name.c_str());
    }
    if (std::string(property_name) == "Discoverable") {
        return g_variant_new_boolean(TRUE);
    }
    return nullptr;
}

// -------- Method Handlers --------
// Characteristic: WriteValue
static void on_method_call(GDBusConnection* /*conn*/, const gchar* /*sender*/, const gchar* /*obj_path*/,
    const gchar* iface, const gchar* method, GVariant* params,
    GDBusMethodInvocation* invoc, gpointer /*user_data*/) {
    if (std::string(iface) == "org.bluez.GattCharacteristic1" &&
        std::string(method) == "WriteValue") {

        GVariant* value = nullptr, * options = nullptr;
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

        g_done = true;
        if (data == g_expected_token) {
            g_ok = true;
            g_dbus_method_invocation_return_value(invoc, nullptr); // success
        }
        else {
            g_ok = false;
            g_dbus_method_invocation_return_dbus_error(
                invoc, "com.sca.Error.InvalidPayload", "Invalid payload for ACCESS");
        }

        if (options) g_variant_unref(options);
        if (value) g_variant_unref(value);
        return;
    }

    // Advertisement: Release (optional)
    if (std::string(iface) == "org.bluez.LEAdvertisement1" &&
        std::string(method) == "Release") {
        // BlueZ가 광고 언레지스터할 때 호출할 수 있음.
        g_dbus_method_invocation_return_value(invoc, nullptr);
        return;
    }

    g_dbus_method_invocation_return_dbus_error(
        invoc, "org.freedesktop.DBus.Error.UnknownMethod", "Unknown method");
}

// -------- VTables (static: lifetime-stable) --------
static const GDBusInterfaceVTable SERVICE_VTABLE = {
    /* method_call */ nullptr,
    /* get_property*/ service_get,
    /* set_property*/ nullptr
};
static const GDBusInterfaceVTable CHAR_VTABLE = {
    /* method_call */ on_method_call,
    /* get_property*/ char_get,
    /* set_property*/ nullptr
};
static const GDBusInterfaceVTable ADV_VTABLE = {
    /* method_call */ on_method_call, // Release() 처리
    /* get_property*/ adv_get,
    /* set_property*/ nullptr
};

int main(int argc, char** argv) {
    // ---- Args ----
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
        else { std::printf("usage: %s --hash <16-hex> [--name NAME] [--timeout SEC] [--token STR] [--require-encrypt 0|1]\n", argv[0]); return 2; }
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

    // Set Adapter Alias
    g_dbus_connection_call_sync(conn, "org.bluez", adapterPath.c_str(),
        "org.freedesktop.DBus.Properties", "Set",
        g_variant_new("(ssv)", "org.bluez.Adapter1", "Alias",
            g_variant_new_string(g_local_name.c_str())),
        nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);

    // ---- Parse interface XMLs (keep lifetime; unref at end) ----
    GDBusNodeInfo* service_node = g_dbus_node_info_new_for_xml(SERVICE_IF_XML, &err);
    if (!service_node) { std::cerr << "[ERR] service xml: " << err->message << "\n"; g_error_free(err); return 2; }
    GDBusNodeInfo* char_node = g_dbus_node_info_new_for_xml(CHAR_IF_XML, &err);
    if (!char_node) { std::cerr << "[ERR] char xml: " << err->message << "\n"; g_error_free(err); return 2; }
    GDBusNodeInfo* adv_node = g_dbus_node_info_new_for_xml(ADV_IF_XML, &err);
    if (!adv_node) { std::cerr << "[ERR] adv xml: " << err->message << "\n"; g_error_free(err); return 2; }

    // ---- Export objects ----
    guint reg_service = g_dbus_connection_register_object(
        conn, SERVICE_PATH, service_node->interfaces[0],
        &SERVICE_VTABLE, nullptr, nullptr, &err);
    if (!reg_service) { std::cerr << "[ERR] register service: " << (err ? err->message : "unknown") << "\n"; if (err) g_error_free(err); return 2; }

    guint reg_char = g_dbus_connection_register_object(
        conn, CHAR_PATH, char_node->interfaces[0],
        &CHAR_VTABLE, nullptr, nullptr, &err);
    if (!reg_char) { std::cerr << "[ERR] register char: " << (err ? err->message : "unknown") << "\n"; if (err) g_error_free(err); return 2; }

    // LEAdvertisement1 object export (so BlueZ can introspect it)
    guint reg_adv = g_dbus_connection_register_object(
        conn, ADV_PATH, adv_node->interfaces[0],
        &ADV_VTABLE, nullptr, nullptr, &err);
    if (!reg_adv) { std::cerr << "[ERR] register adv: " << (err ? err->message : "unknown") << "\n"; if (err) g_error_free(err); return 2; }

    // ---- Register GATT Application (to adapter path) ----
    GVariant* ret = g_dbus_connection_call_sync(
        conn, "org.bluez", adapterPath.c_str(),
        "org.bluez.GattManager1", "RegisterApplication",
        g_variant_new("(oa{sv})", APP_PATH, g_variant_new("a{sv}", nullptr)),
        nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &err);
    if (!ret) { std::cerr << "[ERR] RegisterApplication: " << (err ? err->message : "unknown") << "\n"; if (err) g_error_free(err); return 2; }
    g_variant_unref(ret);

    // ---- Register Advertisement (to adapter path) ----
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

    // ---- Main loop (wait for write or timeout) ----
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
        g_variant_new("(o)", ADV_PATH), nullptr,
        G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);

    g_dbus_connection_unregister_object(conn, reg_adv);
    g_dbus_connection_unregister_object(conn, reg_char);
    g_dbus_connection_unregister_object(conn, reg_service);

    g_dbus_node_info_unref(adv_node);
    g_dbus_node_info_unref(char_node);
    g_dbus_node_info_unref(service_node);

    return (g_done && g_ok) ? 0 : 1;
}
