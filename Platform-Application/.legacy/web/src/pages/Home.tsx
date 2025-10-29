import { Link } from "react-router-dom";
import { Calendar, User, Car, ChevronRight, Shield, MapPin } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import StatusBadge from "@/components/StatusBadge";

export default function Home() {
  // Mock data
  const activeRental = {
    id: "R-001",
    vehicle: "SEDAN-24",
    location: "SEOUL-01",
    time: "10:00~12:00",
    status: "approved" as const,
  };

  const mfaStatus = {
    registered: false,
    status: "pending" as const,
  };

  return (
    <div className="container mx-auto p-4 space-y-6 max-w-2xl">
      {/* Welcome Section */}
      <div className="text-center space-y-2 py-6">
        <h2 className="text-3xl font-bold bg-gradient-to-r from-primary via-primary-glow to-primary bg-clip-text text-transparent">
          환영합니다
        </h2>
        <p className="text-muted-foreground">내 몸에 맞춘 개인화 주행을 시작하세요</p>
      </div>

      {/* Active Rental */}
      {activeRental && (
        <Card className="border-border/50 bg-gradient-card shadow-lg">
          <CardHeader>
            <div className="flex items-center justify-between">
              <CardTitle className="flex items-center gap-2">
                <Calendar className="w-5 h-5 text-primary" />
                예약 현황
              </CardTitle>
              <StatusBadge type="rental" status={activeRental.status} />
            </div>
          </CardHeader>
          <CardContent className="space-y-4">
            <div className="flex items-center justify-between p-4 rounded-lg bg-secondary/30">
              <div className="space-y-2">
                <div className="flex items-center gap-2">
                  <Car className="w-4 h-4 text-primary" />
                  <span className="font-semibold text-lg">{activeRental.vehicle}</span>
                </div>
                <div className="flex items-center gap-2 text-sm text-muted-foreground">
                  <MapPin className="w-3.5 h-3.5" />
                  {activeRental.location}
                </div>
                <p className="text-sm text-foreground">{activeRental.time}</p>
              </div>
              <Link to={`/rental/${activeRental.id}`}>
                <Button variant="ghost" size="icon">
                  <ChevronRight className="w-5 h-5" />
                </Button>
              </Link>
            </div>

            <div className="p-4 rounded-lg bg-warning/10 border border-warning/20">
              <p className="text-sm text-warning-foreground">
                차량 탑승 시 자동으로 MFA 인증이 진행됩니다
              </p>
            </div>
          </CardContent>
        </Card>
      )}

      {/* MFA Status */}
      <Card className="border-border/50 bg-gradient-card shadow-lg">
        <CardHeader>
          <div className="flex items-center justify-between">
            <CardTitle className="flex items-center gap-2">
              <Shield className="w-5 h-5 text-primary" />
              MFA 상태
            </CardTitle>
            <StatusBadge type="mfa" status={mfaStatus.status} />
          </div>
        </CardHeader>
        <CardContent className="space-y-4">
          {!mfaStatus.registered ? (
            <>
              <div className="p-4 rounded-lg bg-destructive/10 border border-destructive/20">
                <p className="text-sm text-destructive-foreground mb-2">
                  사전 등록이 필요합니다
                </p>
                <p className="text-xs text-muted-foreground">
                  차량 인증을 위해 셀카, 체형, NFC/BT 정보를 등록해주세요
                </p>
              </div>
              <Link to="/register">
                <Button variant="hero" className="w-full">
                  사전 등록하기
                </Button>
              </Link>
            </>
          ) : (
            <div className="p-4 rounded-lg bg-success/10 border border-success/20">
              <p className="text-sm text-success-foreground">
                등록 완료! 차량 탑승 시 자동 인증됩니다
              </p>
            </div>
          )}
        </CardContent>
      </Card>

      {/* Quick Actions */}
      <Card className="border-border/50 bg-gradient-card shadow-lg">
        <CardHeader>
          <CardTitle>빠른 액션</CardTitle>
          <CardDescription>자주 사용하는 기능</CardDescription>
        </CardHeader>
        <CardContent>
          <div className="grid grid-cols-2 gap-3">
            <Link to="/rentals" className="block">
              <Button variant="outline" className="w-full h-20 flex-col gap-2">
                <Car className="w-6 h-6" />
                <span>차량 대여</span>
              </Button>
            </Link>
            <Link to="/register" className="block">
              <Button variant="outline" className="w-full h-20 flex-col gap-2">
                <User className="w-6 h-6" />
                <span>프로필 등록</span>
              </Button>
            </Link>
          </div>
        </CardContent>
      </Card>
    </div>
  );
}
