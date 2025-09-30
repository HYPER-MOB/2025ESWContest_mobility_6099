import { useState } from "react";
import { Shield, AlertTriangle, Check, Armchair, ScanFace, ChevronUp, ChevronDown, ChevronLeft, ChevronRight } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Slider } from "@/components/ui/slider";
import { Label } from "@/components/ui/label";
import StatusBadge from "@/components/StatusBadge";
import { useToast } from "@/hooks/use-toast";

export default function Control() {
  const { toast } = useToast();
  const [mfaPassed, setMfaPassed] = useState(false);
  const [safetyOk, setSafetyOk] = useState(true);

  const [seatPosition, setSeatPosition] = useState({
    slide: 0,
    height: 2,
    backrest: 105,
  });

  const handleSeatAdjust = () => {
    toast({
      title: "시트 조정 완료",
      description: "명령이 성공적으로 전송되었습니다",
    });
  };

  const handleMirrorAdjust = (direction: string) => {
    toast({
      title: "미러 조정",
      description: `${direction} 방향으로 조정 중...`,
    });
  };

  const handleApplyProfile = () => {
    setMfaPassed(true);
    toast({
      title: "프로필 적용 완료",
      description: "등록된 체형에 맞춰 시트와 미러가 조정되었습니다",
    });
  };

  // If MFA not passed, show lock screen
  if (!mfaPassed) {
    return (
      <div className="container mx-auto p-4 space-y-6 max-w-2xl">
        <Card className="border-border/50 bg-gradient-card shadow-lg">
          <CardContent className="p-12 text-center space-y-6">
            <div className="w-20 h-20 mx-auto rounded-full bg-destructive/20 flex items-center justify-center">
              <Shield className="w-10 h-10 text-destructive" />
            </div>
            
            <div className="space-y-2">
              <h2 className="text-2xl font-bold">제어 불가</h2>
              <p className="text-muted-foreground">
                차량 탑승 후 자동 인증을 완료해주세요
              </p>
            </div>

            <Button variant="outline" onClick={handleApplyProfile}>
              인증 시뮬레이션 (데모)
            </Button>
          </CardContent>
        </Card>
      </div>
    );
  }

  // If safety blocked
  if (!safetyOk) {
    return (
      <div className="container mx-auto p-4 space-y-6 max-w-2xl">
        <Card className="border-border/50 bg-gradient-card shadow-lg">
          <CardContent className="p-12 text-center space-y-6">
            <div className="w-20 h-20 mx-auto rounded-full bg-warning/20 flex items-center justify-center">
              <AlertTriangle className="w-10 h-10 text-warning" />
            </div>
            
            <div className="space-y-2">
              <h2 className="text-2xl font-bold">제어 일시 정지</h2>
              <p className="text-muted-foreground">
                • 주행 중입니다<br />
                • 정차 후 다시 시도해주세요
              </p>
            </div>

            <Button variant="outline" onClick={() => setSafetyOk(true)}>
              안전 모드 해제 (데모)
            </Button>
          </CardContent>
        </Card>
      </div>
    );
  }

  return (
    <div className="container mx-auto p-4 space-y-6 max-w-2xl pb-20">
      {/* Status Header */}
      <Card className="border-success/50 bg-gradient-card shadow-lg">
        <CardContent className="p-4">
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-3">
              <div className="w-10 h-10 rounded-full bg-success/20 flex items-center justify-center">
                <Check className="w-5 h-5 text-success" />
              </div>
              <div>
                <p className="font-medium">제어 가능</p>
                <p className="text-xs text-muted-foreground">MFA 인증 완료</p>
              </div>
            </div>
            <div className="flex gap-2">
              <StatusBadge type="mfa" status="passed" />
              <StatusBadge type="safety" status="ok" />
            </div>
          </div>
        </CardContent>
      </Card>

      {/* Profile */}
      <Card className="border-border/50 bg-gradient-card shadow-lg">
        <CardHeader>
          <CardTitle className="flex items-center gap-2">
            <ScanFace className="w-5 h-5 text-primary" />
            프로필
          </CardTitle>
          <CardDescription>등록된 체형 프로필을 적용합니다</CardDescription>
        </CardHeader>
        <CardContent>
          <Button variant="hero" className="w-full" onClick={handleApplyProfile}>
            프로필 자동 적용
          </Button>
        </CardContent>
      </Card>

      {/* Seat Control */}
      <Card className="border-border/50 bg-gradient-card shadow-lg">
        <CardHeader>
          <CardTitle className="flex items-center gap-2">
            <Armchair className="w-5 h-5 text-primary" />
            시트 제어
          </CardTitle>
          <CardDescription>시트 위치를 미세 조정합니다</CardDescription>
        </CardHeader>
        <CardContent className="space-y-6">
          <div className="space-y-3">
            <Label>슬라이드: {seatPosition.slide}</Label>
            <Slider
              value={[seatPosition.slide]}
              onValueChange={(v) => setSeatPosition({ ...seatPosition, slide: v[0] })}
              min={-10}
              max={10}
              step={1}
              className="w-full"
            />
          </div>

          <div className="space-y-3">
            <Label>높이: {seatPosition.height}</Label>
            <Slider
              value={[seatPosition.height]}
              onValueChange={(v) => setSeatPosition({ ...seatPosition, height: v[0] })}
              min={0}
              max={5}
              step={1}
              className="w-full"
            />
          </div>

          <div className="space-y-3">
            <Label>등받이 각도: {seatPosition.backrest}°</Label>
            <Slider
              value={[seatPosition.backrest]}
              onValueChange={(v) => setSeatPosition({ ...seatPosition, backrest: v[0] })}
              min={90}
              max={120}
              step={1}
              className="w-full"
            />
          </div>

          <Button variant="default" className="w-full" onClick={handleSeatAdjust}>
            시트 조정 적용
          </Button>
        </CardContent>
      </Card>

      {/* Mirror Control */}
      <Card className="border-border/50 bg-gradient-card shadow-lg">
        <CardHeader>
          <CardTitle>미러 제어</CardTitle>
          <CardDescription>좌/우 미러를 8방향으로 조정합니다</CardDescription>
        </CardHeader>
        <CardContent className="space-y-6">
          {/* Left Mirror */}
          <div className="space-y-3">
            <Label>좌측 미러</Label>
            <div className="grid grid-cols-3 gap-2 max-w-[180px] mx-auto">
              <Button variant="outline" size="icon" onClick={() => handleMirrorAdjust("좌상")}>
                <ChevronUp className="w-4 h-4 -rotate-45" />
              </Button>
              <Button variant="outline" size="icon" onClick={() => handleMirrorAdjust("상")}>
                <ChevronUp className="w-4 h-4" />
              </Button>
              <Button variant="outline" size="icon" onClick={() => handleMirrorAdjust("우상")}>
                <ChevronUp className="w-4 h-4 rotate-45" />
              </Button>
              
              <Button variant="outline" size="icon" onClick={() => handleMirrorAdjust("좌")}>
                <ChevronLeft className="w-4 h-4" />
              </Button>
              <div className="w-10 h-10 bg-primary/10 rounded-md flex items-center justify-center">
                <div className="w-2 h-2 bg-primary rounded-full" />
              </div>
              <Button variant="outline" size="icon" onClick={() => handleMirrorAdjust("우")}>
                <ChevronRight className="w-4 h-4" />
              </Button>
              
              <Button variant="outline" size="icon" onClick={() => handleMirrorAdjust("좌하")}>
                <ChevronDown className="w-4 h-4 rotate-45" />
              </Button>
              <Button variant="outline" size="icon" onClick={() => handleMirrorAdjust("하")}>
                <ChevronDown className="w-4 h-4" />
              </Button>
              <Button variant="outline" size="icon" onClick={() => handleMirrorAdjust("우하")}>
                <ChevronDown className="w-4 h-4 -rotate-45" />
              </Button>
            </div>
          </div>

          {/* Right Mirror */}
          <div className="space-y-3">
            <Label>우측 미러</Label>
            <div className="grid grid-cols-3 gap-2 max-w-[180px] mx-auto">
              <Button variant="outline" size="icon" onClick={() => handleMirrorAdjust("좌상")}>
                <ChevronUp className="w-4 h-4 -rotate-45" />
              </Button>
              <Button variant="outline" size="icon" onClick={() => handleMirrorAdjust("상")}>
                <ChevronUp className="w-4 h-4" />
              </Button>
              <Button variant="outline" size="icon" onClick={() => handleMirrorAdjust("우상")}>
                <ChevronUp className="w-4 h-4 rotate-45" />
              </Button>
              
              <Button variant="outline" size="icon" onClick={() => handleMirrorAdjust("좌")}>
                <ChevronLeft className="w-4 h-4" />
              </Button>
              <div className="w-10 h-10 bg-primary/10 rounded-md flex items-center justify-center">
                <div className="w-2 h-2 bg-primary rounded-full" />
              </div>
              <Button variant="outline" size="icon" onClick={() => handleMirrorAdjust("우")}>
                <ChevronRight className="w-4 h-4" />
              </Button>
              
              <Button variant="outline" size="icon" onClick={() => handleMirrorAdjust("좌하")}>
                <ChevronDown className="w-4 h-4 rotate-45" />
              </Button>
              <Button variant="outline" size="icon" onClick={() => handleMirrorAdjust("하")}>
                <ChevronDown className="w-4 h-4" />
              </Button>
              <Button variant="outline" size="icon" onClick={() => handleMirrorAdjust("우하")}>
                <ChevronDown className="w-4 h-4 -rotate-45" />
              </Button>
            </div>
          </div>
        </CardContent>
      </Card>

      {/* Recent Logs */}
      <Card className="border-border/50 bg-gradient-card shadow-lg">
        <CardHeader>
          <CardTitle>실시간 로그</CardTitle>
        </CardHeader>
        <CardContent>
          <div className="space-y-2 text-sm font-mono">
            <div className="flex items-center gap-2">
              <span className="text-success">✓</span>
              <span className="text-muted-foreground">10:01</span>
              <span>seat: completed</span>
            </div>
            <div className="flex items-center gap-2">
              <span className="text-success">✓</span>
              <span className="text-muted-foreground">10:02</span>
              <span>mirror: completed</span>
            </div>
            <div className="flex items-center gap-2">
              <span className="text-primary">→</span>
              <span className="text-muted-foreground">10:03</span>
              <span>profile: applied</span>
            </div>
          </div>
        </CardContent>
      </Card>
    </div>
  );
}
