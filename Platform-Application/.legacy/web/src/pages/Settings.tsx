import { User, Smartphone, Bell, Shield, FileText, Info, ChevronRight } from "lucide-react";
import { Card, CardContent } from "@/components/ui/card";

const settingsItems = [
  { icon: User, label: "계정 / 로그인", path: "/settings/account" },
  { icon: Smartphone, label: "디바이스 정보 (NFC/BT)", path: "/settings/device" },
  { icon: Bell, label: "알림 설정", path: "/settings/notifications" },
  { icon: Shield, label: "개인정보 / 약관", path: "/settings/privacy" },
  { icon: Info, label: "버전 정보", path: "/settings/about" },
];

export default function Settings() {
  return (
    <div className="container mx-auto p-4 space-y-6 max-w-2xl">
      <div className="space-y-2">
        <h1 className="text-2xl font-bold">설정</h1>
        <p className="text-muted-foreground">앱 설정 및 정보</p>
      </div>

      <div className="space-y-2">
        {settingsItems.map((item) => {
          const Icon = item.icon;
          return (
            <Card
              key={item.path}
              className="border-border/50 bg-gradient-card hover:border-primary/50 transition-smooth cursor-pointer group"
            >
              <CardContent className="p-4">
                <div className="flex items-center justify-between">
                  <div className="flex items-center gap-3">
                    <div className="w-10 h-10 rounded-lg bg-secondary/50 flex items-center justify-center group-hover:bg-primary/10 transition-smooth">
                      <Icon className="w-5 h-5 text-muted-foreground group-hover:text-primary transition-smooth" />
                    </div>
                    <span className="font-medium">{item.label}</span>
                  </div>
                  <ChevronRight className="w-5 h-5 text-muted-foreground group-hover:text-primary transition-smooth" />
                </div>
              </CardContent>
            </Card>
          );
        })}
      </div>

      <Card className="border-border/50 bg-card/50">
        <CardContent className="p-6 text-center space-y-2">
          <p className="text-sm text-muted-foreground">MyDrive3DX Mobile</p>
          <p className="text-xs text-muted-foreground">Version 1.0.0</p>
        </CardContent>
      </Card>
    </div>
  );
}
