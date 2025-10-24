#pragma once
#include <string>
#include <functional>
#include <gio/gio.h>
#include <glib.h>

/*
 * BlueZ GATT Peripheral를 프로세스 내에서 직접 구동하는 모듈.
 * 기존 단독 실행파일(main) 형태였던 코드를 라이브러리화함.
 *
 * 동작:
 *  - 지정한 Service UUID와 Characteristic(UUID 고정)로 Peripheral 등록
 *  - 스마트폰이 Char에 쓰기(Write)하면 콜백/결과 통해 전달
 *  - 타임아웃/정리까지 내부에서 수행
 */

namespace sca {

    struct BleConfig {
        std::string hash12;          // UUID의 마지막 12자리(예: "A1B2C3D4E5F6")
        std::string local_name{ "SCA-CAR" };
        std::string expected_token{ "ACCESS" };
        int         timeout_sec{ 30 };
        bool        require_encrypt{ false };
    };

    struct BleResult {
        bool ok{ false };              // expected_token과 일치하는 데이터가 쓰이면 true
        bool wrote{ false };           // 어떤 값이든 Write 이벤트가 발생했는지
        std::string written_data;    // 클라이언트가 쓴 데이터(그대로 텍스트로)
        std::string adapter_path;    // 사용된 어댑터 경로
    };

    class BlePeripheral {
    public:
        BlePeripheral();
        ~BlePeripheral();

        // 동기 실행: 등록 → 광고 → (Write 수신 or 타임아웃) → 정리 → 결과 반환
        // 성공(토큰일치) 시 true, 아니면 false
        bool run(const BleConfig& cfg, BleResult& out);

    private:
        // 내부 상태
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

        // 내부 유틸
        static std::string build_service_uuid(const std::string& hash12);
        std::string get_adapter_path();
        bool call_set(const std::string& objpath, const char* iface, const char* prop, GVariant* v);

        // 등록/해제
        bool export_objects();
        void unexport_objects();
        bool register_app(const std::string& adapter);
        bool register_adv(const std::string& adapter);
        void unregister_adv(const std::string& adapter);

        // 콜백/정리
        void quit_loop(bool ok);

        // 정적 핸들러에서 this를 쓰기 위해 static 보관
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

        // 내부 플래그
        bool done_{ false };
        bool ok_{ false };
    };

} // namespace sca
