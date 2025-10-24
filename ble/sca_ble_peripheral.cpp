#include "sca_ble_peripheral.hpp"
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <cctype>

namespace sca {

    BlePeripheral* BlePeripheral::s_self = nullptr;

    static const char* ACCESS_CHAR_UUID = "c0de0001-0000-1000-8000-000000000001";

    const char* BlePeripheral::SERVICE_IF_XML = R"XML(
<node>
  <interface name="org.bluez.GattService1">
    <property name="UUID" type="s" access="read"/>
    <property name="Primary" type="b" access="read"/>
    <property name="Includes" type="ao" access="read"/>
  </interface>
</node>
)XML";

    const char* BlePeripheral::CHAR_IF_XML = R"XML(
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

    const char* BlePeripheral::ADV_IF_XML = R"XML(
<node>
  <interface name="org.bluez.LEAdvertisement1">
    <method name="Release"/>
    <property name="Type"         type="s"  access="read"/>
    <property name="ServiceUUIDs" type="as" access="read"/>
    <property name="Includes"     type="as" access="read"/>
  </interface>
</node>
)XML";

    const char* BlePeripheral::OBJMGR_IF_XML = R"XML(
<node>
  <interface name="org.freedesktop.DBus.ObjectManager">
    <method name="GetManagedObjects">
      <arg type="a{oa{sa{sv}}}" direction="out"/>
    </method>
  </interface>
</node>
)XML";

    const GDBusInterfaceVTable BlePeripheral::SERVICE_VTABLE = { nullptr, BlePeripheral::service_get, nullptr };
    const GDBusInterfaceVTable BlePeripheral::CHAR_VTABLE = { BlePeripheral::char_method_call, BlePeripheral::char_get, nullptr };
    const GDBusInterfaceVTable BlePeripheral::ADV_VTABLE = { BlePeripheral::adv_method_call,  BlePeripheral::adv_get,  nullptr };
    const GDBusInterfaceVTable BlePeripheral::OBJMGR_VTABLE = { BlePeripheral::objmgr_method_call, nullptr, nullptr };

    BlePeripheral::BlePeripheral() { s_self = this; }
    BlePeripheral::~BlePeripheral() { s_self = nullptr; }

    std::string BlePeripheral::build_service_uuid(const std::string& hash12) {
        if (hash12.size() != 12) {
            std::cerr << "[BLE] invalid hash length; need 12 hex\n"; std::exit(2);
        }
        std::string up = hash12;
        for (auto& c : up) c = (char)std::toupper((unsigned char)c);
        return "12345678-0000-1000-8000-" + up;
    }

    std::string BlePeripheral::get_adapter_path() {
        GError* err = nullptr;
        GVariant* ret = g_dbus_connection_call_sync(
            conn_, "org.bluez", "/",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects",
            nullptr, nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &err);
        if (!ret) {
            std::cerr << "[BLE] GetManagedObjects failed: " << (err ? err->message : "unknown") << "\n";
            if (err) g_error_free(err);
            return {};
        }

        std::string out;
        if (g_variant_is_of_type(ret, G_VARIANT_TYPE_TUPLE)) {
            GVariant* dict = g_variant_get_child_value(ret, 0);
            GVariantIter* i = nullptr;
            g_variant_get(dict, "a{oa{sa{sv}}}", &i);
            const gchar* objpath = nullptr; GVariant* ifmap = nullptr;
            while (g_variant_iter_loop(i, "{&o@a{sa{sv}}}", &objpath, &ifmap)) {
                GVariantIter* ii = nullptr; const gchar* ifname = nullptr; GVariant* props = nullptr;
                g_variant_get(ifmap, "a{sa{sv}}", &ii);
                while (g_variant_iter_loop(ii, "{&s@a{sv}}", &ifname, &props)) {
                    if (g_strcmp0(ifname, "org.bluez.Adapter1") == 0)
                        out = objpath;
                }
                if (ii) g_variant_iter_free(ii);
            }
            if (i) g_variant_iter_free(i);
            g_variant_unref(dict);
        }
        g_variant_unref(ret);
        return out;
    }

    bool BlePeripheral::call_set(const std::string& objpath, const char* iface, const char* prop, GVariant* v) {
        GError* err = nullptr;
        GVariant* r = g_dbus_connection_call_sync(
            conn_, "org.bluez", objpath.c_str(),
            "org.freedesktop.DBus.Properties", "Set",
            g_variant_new("(ssv)", iface, prop, v),
            nullptr, G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, &err);
        if (!r) {
            std::cerr << "[BLE] Set(" << iface << "." << prop << ") failed: " << (err ? err->message : "unknown") << "\n";
            if (err) g_error_free(err);
            return false;
        }
        g_variant_unref(r);
        return true;
    }

    void BlePeripheral::objmgr_method_call(GDBusConnection* c, const gchar* sender, const gchar* obj_path,
        const gchar* iface, const gchar* method, GVariant* params,
        GDBusMethodInvocation* inv, gpointer) {
        auto* self = BlePeripheral::s_self;
        if (!self) return;
        if (g_strcmp0(iface, "org.freedesktop.DBus.ObjectManager") == 0 &&
            g_strcmp0(method, "GetManagedObjects") == 0) {

            GVariantBuilder root; g_variant_builder_init(&root, G_VARIANT_TYPE("a{oa{sa{sv}}}"));

            // service
            {
                GVariantBuilder ifmap; g_variant_builder_init(&ifmap, G_VARIANT_TYPE("a{sa{sv}}"));
                GVariantBuilder props; g_variant_builder_init(&props, G_VARIANT_TYPE("a{sv}"));
                g_variant_builder_add(&props, "{sv}", "UUID", g_variant_new_string(self->service_uuid_.c_str()));
                g_variant_builder_add(&props, "{sv}", "Primary", g_variant_new_boolean(TRUE));
                g_variant_builder_add(&props, "{sv}", "Includes", g_variant_new("ao", NULL));
                g_variant_builder_add(&ifmap, "{s@a{sv}}", "org.bluez.GattService1", g_variant_builder_end(&props));
                g_variant_builder_add(&root, "{o@a{sa{sv}}}", self->SERVICE_PATH, g_variant_builder_end(&ifmap));
            }

            // char
            {
                GVariantBuilder ifmap; g_variant_builder_init(&ifmap, G_VARIANT_TYPE("a{sa{sv}}"));
                GVariantBuilder props; g_variant_builder_init(&props, G_VARIANT_TYPE("a{sv}"));
                g_variant_builder_add(&props, "{sv}", "UUID", g_variant_new_string(ACCESS_CHAR_UUID));
                g_variant_builder_add(&props, "{sv}", "Service", g_variant_new_object_path(self->SERVICE_PATH));
                const char* flags_arr[] = { "write", "write-without-response", nullptr };
                g_variant_builder_add(&props, "{sv}", "Flags", g_variant_new_strv(flags_arr, -1));
                g_variant_builder_add(&props, "{sv}", "Descriptors", g_variant_new("ao", NULL));
                g_variant_builder_add(&ifmap, "{s@a{sv}}", "org.bluez.GattCharacteristic1", g_variant_builder_end(&props));
                g_variant_builder_add(&root, "{o@a{sa{sv}}}", self->CHAR_PATH, g_variant_builder_end(&ifmap));
            }

            GVariant* dict = g_variant_builder_end(&root);
            g_dbus_method_invocation_return_value(inv, g_variant_new("(@a{oa{sa{sv}}})", dict));
            return;
        }
        g_dbus_method_invocation_return_dbus_error(inv, "org.freedesktop.DBus.Error.UnknownMethod", "Unknown");
    }

    GVariant* BlePeripheral::service_get(GDBusConnection*, const gchar*, const gchar*, const gchar*,
        const gchar* prop, GError**, gpointer) {
        auto* self = BlePeripheral::s_self; if (!self) return nullptr;
        if (!g_strcmp0(prop, "UUID"))     return g_variant_new_string(self->service_uuid_.c_str());
        if (!g_strcmp0(prop, "Primary"))  return g_variant_new_boolean(TRUE);
        if (!g_strcmp0(prop, "Includes")) return g_variant_new("ao", nullptr);
        return nullptr;
    }

    GVariant* BlePeripheral::char_get(GDBusConnection*, const gchar*, const gchar*, const gchar*,
        const gchar* prop, GError**, gpointer) {
        auto* self = BlePeripheral::s_self; if (!self) return nullptr;
        if (!g_strcmp0(prop, "UUID"))    return g_variant_new_string(ACCESS_CHAR_UUID);
        if (!g_strcmp0(prop, "Service")) return g_variant_new_object_path(self->SERVICE_PATH);
        if (!g_strcmp0(prop, "Flags")) {
            if (self->cfg_.require_encrypt) {
                const gchar* flags[] = { "write","encrypt-write",nullptr };
                return g_variant_new_strv(flags, -1);
            }
            else {
                const gchar* flags[] = { "write",nullptr };
                return g_variant_new_strv(flags, -1);
            }
        }
        if (!g_strcmp0(prop, "Descriptors")) return g_variant_new("ao", nullptr);
        return nullptr;
    }

    GVariant* BlePeripheral::adv_get(GDBusConnection*, const gchar*, const gchar*, const gchar*,
        const gchar* prop, GError**, gpointer) {
        auto* self = BlePeripheral::s_self; if (!self) return nullptr;
        if (g_strcmp0(prop, "Type") == 0)         return g_variant_new_string("peripheral");
        if (g_strcmp0(prop, "ServiceUUIDs") == 0) {
            const char* u = self->service_uuid_.c_str();
            return g_variant_new_strv(&u, 1);
        }
        if (g_strcmp0(prop, "Includes") == 0) {
            const gchar* inc[] = { "local-name", nullptr };
            return g_variant_new_strv(inc, -1);
        }
        return nullptr;
    }

    void BlePeripheral::char_method_call(GDBusConnection*, const gchar*, const gchar*, const gchar*,
        const gchar* method, GVariant* params,
        GDBusMethodInvocation* inv, gpointer) {
        auto* self = BlePeripheral::s_self; if (!self) return;
        if (!g_strcmp0(method, "WriteValue")) {
            GVariant* value = nullptr; GVariant* options = nullptr;
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

            self->res_.wrote = true;
            self->res_.written_data = data;
            if (data == self->cfg_.expected_token) {
                self->ok_ = true;
                self->res_.ok = true;
                g_dbus_method_invocation_return_value(inv, nullptr);
            }
            else {
                self->ok_ = false;
                self->res_.ok = false;
                g_dbus_method_invocation_return_dbus_error(inv, "com.sca.Error.InvalidPayload", "Invalid payload");
            }

            if (options) g_variant_unref(options);
            if (value)   g_variant_unref(value);

            // 종료
            self->quit_loop(self->res_.ok);
            return;
        }
        if (!g_strcmp0(method, "ReadValue")) {
            GVariant* options = nullptr; g_variant_get(params, "(@a{sv})", &options);
            if (options) g_variant_unref(options);
            GVariant* empty = g_variant_new_from_data(G_VARIANT_TYPE("ay"), "", 0, TRUE, nullptr, nullptr);
            g_dbus_method_invocation_return_value(inv, g_variant_new_tuple(&empty, 1));
            return;
        }
        g_dbus_method_invocation_return_dbus_error(inv, "org.freedesktop.DBus.Error.UnknownMethod", "Unknown");
    }

    void BlePeripheral::adv_method_call(GDBusConnection*, const gchar*, const gchar*,
        const gchar* iface, const gchar* method,
        GVariant*, GDBusMethodInvocation* inv, gpointer) {
        if (!g_strcmp0(iface, "org.bluez.LEAdvertisement1") && !g_strcmp0(method, "Release")) {
            g_dbus_method_invocation_return_value(inv, nullptr);
            return;
        }
        g_dbus_method_invocation_return_dbus_error(inv, "org.freedesktop.DBus.Error.UnknownMethod", "Unknown");
    }

    bool BlePeripheral::export_objects() {
        GError* err = nullptr;
        GDBusNodeInfo* service_node = g_dbus_node_info_new_for_xml(SERVICE_IF_XML, &err);
        if (!service_node) { std::cerr << "[BLE] service xml: " << err->message << "\n"; g_error_free(err); return false; }
        GDBusNodeInfo* char_node = g_dbus_node_info_new_for_xml(CHAR_IF_XML, &err);
        if (!char_node) { std::cerr << "[BLE] char xml: " << err->message << "\n"; g_error_free(err); g_dbus_node_info_unref(service_node); return false; }
        GDBusNodeInfo* adv_node = g_dbus_node_info_new_for_xml(ADV_IF_XML, &err);
        if (!adv_node) { std::cerr << "[BLE] adv xml: " << err->message << "\n"; g_error_free(err); g_dbus_node_info_unref(char_node); g_dbus_node_info_unref(service_node); return false; }
        GDBusNodeInfo* objmgr_node = g_dbus_node_info_new_for_xml(OBJMGR_IF_XML, &err);
        if (!objmgr_node) { std::cerr << "[BLE] objmgr xml: " << err->message << "\n"; g_error_free(err); g_dbus_node_info_unref(adv_node); g_dbus_node_info_unref(char_node); g_dbus_node_info_unref(service_node); return false; }

        reg_service_ = g_dbus_connection_register_object(conn_, SERVICE_PATH, service_node->interfaces[0], &SERVICE_VTABLE, nullptr, nullptr, &err);
        if (!reg_service_) { std::cerr << "[BLE] reg service: " << (err ? err->message : "unknown") << "\n"; if (err)g_error_free(err); return false; }
        reg_char_ = g_dbus_connection_register_object(conn_, CHAR_PATH, char_node->interfaces[0], &CHAR_VTABLE, nullptr, nullptr, &err);
        if (!reg_char_) { std::cerr << "[BLE] reg char: " << (err ? err->message : "unknown") << "\n"; if (err)g_error_free(err); return false; }
        reg_adv_ = g_dbus_connection_register_object(conn_, ADV_PATH, adv_node->interfaces[0], &ADV_VTABLE, nullptr, nullptr, &err);
        if (!reg_adv_) { std::cerr << "[BLE] reg adv: " << (err ? err->message : "unknown") << "\n"; if (err)g_error_free(err); return false; }
        reg_objmgr_ = g_dbus_connection_register_object(conn_, APP_PATH, objmgr_node->interfaces[0], &OBJMGR_VTABLE, nullptr, nullptr, &err);
        if (!reg_objmgr_) { std::cerr << "[BLE] reg objmgr: " << (err ? err->message : "unknown") << "\n"; if (err)g_error_free(err); return false; }

        // 노드정보는 등록 후 해제해도 됨 (bluez가 참조 보관)
        g_dbus_node_info_unref(service_node);
        g_dbus_node_info_unref(char_node);
        g_dbus_node_info_unref(adv_node);
        g_dbus_node_info_unref(objmgr_node);
        return true;
    }

    void BlePeripheral::unexport_objects() {
        if (reg_objmgr_) { g_dbus_connection_unregister_object(conn_, reg_objmgr_); reg_objmgr_ = 0; }
        if (reg_adv_) { g_dbus_connection_unregister_object(conn_, reg_adv_);    reg_adv_ = 0; }
        if (reg_char_) { g_dbus_connection_unregister_object(conn_, reg_char_);   reg_char_ = 0; }
        if (reg_service_) { g_dbus_connection_unregister_object(conn_, reg_service_); reg_service_ = 0; }
    }

    bool BlePeripheral::register_app(const std::string& adapter) {
        bool ok = false;
        GVariant* options = g_variant_new_array(G_VARIANT_TYPE("{sv}"), nullptr, 0);
        GError* err = nullptr;
        g_dbus_connection_call(
            conn_, "org.bluez", adapter.c_str(),
            "org.bluez.GattManager1", "RegisterApplication",
            g_variant_new("(o@a{sv})", APP_PATH, options),
            nullptr, G_DBUS_CALL_FLAGS_NONE, 10000, nullptr,
            [](GObject* src, GAsyncResult* res, gpointer user) {
                GError* e = nullptr;
                GVariant* r = g_dbus_connection_call_finish(G_DBUS_CONNECTION(src), res, &e);
                bool* pok = static_cast<bool*>(user);
                if (!r) { std::cerr << "[BLE] RegisterApplication: " << (e ? e->message : "unknown") << "\n"; if (e)g_error_free(e); *pok = false; }
                else { g_variant_unref(r); *pok = true; }
            },
            &ok);
        while (!ok && g_main_context_iteration(nullptr, TRUE)) { /* wait */ }
        return ok;
    }

    bool BlePeripheral::register_adv(const std::string& adapter) {
        struct AdvRegState { bool ok = false, fail = false; } st;
        GVariant* opts = g_variant_new_array(G_VARIANT_TYPE("{sv}"), nullptr, 0);
        g_dbus_connection_call(
            conn_, "org.bluez", adapter.c_str(),
            "org.bluez.LEAdvertisingManager1", "RegisterAdvertisement",
            g_variant_new("(o@a{sv})", ADV_PATH, opts),
            nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr,
            [](GObject* src, GAsyncResult* res, gpointer user) {
                GError* e = nullptr;
                GVariant* r = g_dbus_connection_call_finish(G_DBUS_CONNECTION(src), res, &e);
                auto* st = static_cast<AdvRegState*>(user);
                if (!r) { std::cerr << "[BLE] RegisterAdvertisement: " << (e ? e->message : "unknown") << "\n"; if (e)g_error_free(e); st->fail = true; }
                else { g_variant_unref(r); st->ok = true; }
            },
            &st);
        while (!st.ok && !st.fail && g_main_context_iteration(nullptr, TRUE)) { /* spin */ }
        return st.ok;
    }

    void BlePeripheral::unregister_adv(const std::string& adapter) {
        g_dbus_connection_call_sync(conn_, "org.bluez", adapter.c_str(),
            "org.bluez.LEAdvertisingManager1", "UnregisterAdvertisement",
            g_variant_new("(o)", ADV_PATH),
            nullptr, G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, nullptr);
    }

    void BlePeripheral::quit_loop(bool ok) {
        ok_ = ok;
        done_ = true;
        if (loop_) g_main_loop_quit(loop_);
    }

    bool BlePeripheral::run(const BleConfig& cfg, BleResult& out) {
        cfg_ = cfg;
        res_ = BleResult{};
        service_uuid_ = build_service_uuid(cfg_.hash12);

        GError* err = nullptr;
        conn_ = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &err);
        if (!conn_) { std::cerr << "[BLE] system bus: " << (err ? err->message : "unknown") << "\n"; if (err)g_error_free(err); return false; }

        std::string adapter = get_adapter_path();
        if (adapter.empty()) { std::cerr << "[BLE] no adapter\n"; return false; }
        res_.adapter_path = adapter;

        // 기본 어댑터 상태
        call_set(adapter, "org.bluez.Adapter1", "Powered", g_variant_new_boolean(TRUE));
        call_set(adapter, "org.bluez.Adapter1", "Discoverable", g_variant_new_boolean(TRUE));
        call_set(adapter, "org.bluez.Adapter1", "Pairable", g_variant_new_boolean(TRUE));
        call_set(adapter, "org.bluez.Adapter1", "Alias", g_variant_new_string(cfg_.local_name.c_str()));

        if (!export_objects()) return false;
        if (!register_app(adapter)) { unexport_objects(); return false; }
        if (!register_adv(adapter)) { unexport_objects(); return false; }

        std::cout << "[BLE] Advertising service " << service_uuid_
            << " name=" << cfg_.local_name
            << " expect=\"" << cfg_.expected_token << "\""
            << " encrypt=" << (cfg_.require_encrypt ? "on" : "off")
            << " timeout=" << cfg_.timeout_sec << "s\n";

        // 타임아웃 타이머
        loop_ = g_main_loop_new(nullptr, FALSE);
        g_timeout_add_seconds(cfg_.timeout_sec, [](gpointer)->gboolean {
            auto* self = BlePeripheral::s_self; if (!self) return G_SOURCE_REMOVE;
            if (!self->done_) {
                std::cerr << "[BLE] timeout reached\n";
                self->res_.ok = false;
                self->quit_loop(false);
            }
            return G_SOURCE_REMOVE;
            }, nullptr);

        // 루프
        g_main_loop_run(loop_);

        // 정리
        unregister_adv(adapter);
        unexport_objects();
        if (loop_) { g_main_loop_unref(loop_); loop_ = nullptr; }
        if (conn_) { g_object_unref(conn_); conn_ = nullptr; }

        out = res_;
        return out.ok;
    }

} // namespace sca
