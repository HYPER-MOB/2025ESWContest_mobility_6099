#pragma once
#include <string>
#include <functional>
#include <gio/gio.h>
#include <glib.h>

namespace sca {

    struct BleConfig {
        std::string hash12;        
        std::string local_name{ "SCA-CAR" };
        std::string expected_token{ "ACCESS" };
        int         timeout_sec{ 30 };
        bool        require_encrypt{ false };
    };

    struct BleResult {
        bool ok{ false };            
        bool wrote{ false };         
        std::string written_data;   
        std::string adapter_path;  
    };

    class BlePeripheral {
    public:
        BlePeripheral();
        ~BlePeripheral();
        bool run(const BleConfig& cfg, BleResult& out);

    private:
        BleConfig cfg_;
        BleResult res_;
        std::string service_uuid_;

        GDBusConnection* conn_{ nullptr };
        GMainLoop* loop_{ nullptr };

        guint reg_service_{ 0 };
        guint reg_char_{ 0 };
        guint reg_adv_{ 0 };
        guint reg_objmgr_{ 0 };

        // DBus Object Paths
        const char* APP_PATH = "/com/sca/app";
        const char* SERVICE_PATH = "/com/sca/app/service0";
        const char* CHAR_PATH = "/com/sca/app/service0/char0";
        const char* ADV_PATH = "/com/sca/app/adv0";

        static std::string build_service_uuid(const std::string& hash12);
        std::string get_adapter_path();
        bool call_set(const std::string& objpath, const char* iface, const char* prop, GVariant* v);

        bool export_objects();
        void unexport_objects();
        bool register_app(const std::string& adapter);
        bool register_adv(const std::string& adapter);
        void unregister_adv(const std::string& adapter);
        void unregister_app(const std::string& adapter);
        void quit_loop(bool ok);

        static BlePeripheral* s_self;

        // ==== DBus XML/VTABLE ====
        static const char* SERVICE_IF_XML;
        static const char* CHAR_IF_XML;
        static const char* ADV_IF_XML;
        static const char* OBJMGR_IF_XML;

        static void objmgr_method_call(GDBusConnection* c, const gchar* sender, const gchar* obj_path,
            const gchar* iface, const gchar* method, GVariant* params,
            GDBusMethodInvocation* inv, gpointer user_data);

        static GVariant* service_get(GDBusConnection*, const gchar*, const gchar*, const gchar*,
            const gchar* prop, GError**, gpointer);
        static GVariant* char_get(GDBusConnection*, const gchar*, const gchar*, const gchar*,
            const gchar* prop, GError**, gpointer);
        static GVariant* adv_get(GDBusConnection*, const gchar*, const gchar*, const gchar*,
            const gchar* prop, GError**, gpointer);

        static void char_method_call(GDBusConnection*, const gchar*, const gchar*, const gchar*,
            const gchar* method, GVariant* params,
            GDBusMethodInvocation* inv, gpointer);
        static void adv_method_call(GDBusConnection*, const gchar*, const gchar*,
            const gchar* iface, const gchar* method,
            GVariant*, GDBusMethodInvocation* inv, gpointer);

        static const GDBusInterfaceVTable SERVICE_VTABLE;
        static const GDBusInterfaceVTable CHAR_VTABLE;
        static const GDBusInterfaceVTable ADV_VTABLE;
        static const GDBusInterfaceVTable OBJMGR_VTABLE;

        bool done_{ false };
        bool ok_{ false };
    };

} // namespace sca
