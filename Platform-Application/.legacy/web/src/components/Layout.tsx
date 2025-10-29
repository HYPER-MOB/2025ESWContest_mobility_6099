import { NavLink, Outlet, useLocation } from "react-router-dom";
import { Home, CalendarClock, IdCard, Car, Settings, LogOut } from "lucide-react";
import { Button } from "@/components/ui/button";
import { cn } from "@/lib/utils";

const tabs = [
  { path: "/", icon: Home, label: "홈" },
  { path: "/rentals", icon: CalendarClock, label: "대여" },
  { path: "/register", icon: IdCard, label: "등록" },
  { path: "/control", icon: Car, label: "제어" },
  { path: "/settings", icon: Settings, label: "설정" },
];

export default function Layout() {
  const location = useLocation();
  
  // Don't show layout on login page
  if (location.pathname === "/login") {
    return <Outlet />;
  }

  const handleLogout = () => {
    localStorage.removeItem("auth");
    window.location.href = "/login";
  };

  return (
    <div className="flex flex-col min-h-screen bg-background">
      {/* Header */}
      <header className="border-b border-border bg-card/50 backdrop-blur-xl sticky top-0 z-50">
        <div className="container mx-auto px-4 h-14 flex items-center justify-between">
          <div className="flex items-center gap-3">
            <div className="w-8 h-8 rounded-lg bg-gradient-primary flex items-center justify-center">
              <Car className="w-5 h-5 text-primary-foreground" />
            </div>
            <h1 className="text-lg font-bold bg-gradient-to-r from-primary via-primary-glow to-primary bg-clip-text text-transparent">
              MyDrive3DX
            </h1>
          </div>

          <Button
            variant="ghost"
            size="icon"
            onClick={handleLogout}
            className="text-muted-foreground hover:text-foreground"
          >
            <LogOut className="w-4 h-4" />
          </Button>
        </div>
      </header>

      {/* Main Content */}
      <main className="flex-1">
        <Outlet />
      </main>

      {/* Bottom Navigation */}
      <nav className="border-t border-border bg-card/80 backdrop-blur-xl sticky bottom-0 z-50">
        <div className="container mx-auto px-2 py-2">
          <div className="flex items-center justify-around">
            {tabs.map((tab) => {
              const isActive = location.pathname === tab.path;
              const Icon = tab.icon;

              return (
                <NavLink
                  key={tab.path}
                  to={tab.path}
                  className={cn(
                    "flex flex-col items-center gap-1 px-4 py-2 rounded-lg transition-smooth min-w-[64px]",
                    isActive
                      ? "bg-primary/10 text-primary"
                      : "text-muted-foreground hover:text-foreground hover:bg-secondary/50"
                  )}
                >
                  <Icon className={cn("w-5 h-5", isActive && "glow-primary")} />
                  <span className="text-xs font-medium">{tab.label}</span>
                </NavLink>
              );
            })}
          </div>
        </div>
      </nav>
    </div>
  );
}
