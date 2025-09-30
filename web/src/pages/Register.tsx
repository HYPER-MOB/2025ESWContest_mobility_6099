import { useState } from "react";
import { Camera, Ruler, Smartphone, Check } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import { useToast } from "@/hooks/use-toast";
import { Progress } from "@/components/ui/progress";

type Step = "selfie" | "body" | "device";

export default function Register() {
  const [currentStep, setCurrentStep] = useState<Step>("selfie");
  const [completedSteps, setCompletedSteps] = useState<Step[]>([]);
  const { toast } = useToast();

  const [bodyMetrics, setBodyMetrics] = useState({
    height: "",
    sitHeight: "",
    shoulderWidth: "",
    armLength: "",
    viewAngle: "",
  });

  const steps = [
    { id: "selfie" as Step, label: "셀카", icon: Camera },
    { id: "body" as Step, label: "체형", icon: Ruler },
    { id: "device" as Step, label: "NFC/BT", icon: Smartphone },
  ];

  const currentStepIndex = steps.findIndex((s) => s.id === currentStep);
  const progress = ((completedSteps.length) / steps.length) * 100;

  const handleStepComplete = (step: Step) => {
    if (!completedSteps.includes(step)) {
      setCompletedSteps([...completedSteps, step]);
    }
    
    const nextIndex = currentStepIndex + 1;
    if (nextIndex < steps.length) {
      setCurrentStep(steps[nextIndex].id);
    } else {
      toast({
        title: "등록 완료",
        description: "MFA 사전 등록이 완료되었습니다",
      });
    }
  };

  const handleBodyMetricsSubmit = () => {
    if (Object.values(bodyMetrics).every((v) => v)) {
      handleStepComplete("body");
    } else {
      toast({
        title: "입력 필요",
        description: "모든 항목을 입력해주세요",
        variant: "destructive",
      });
    }
  };

  return (
    <div className="container mx-auto p-4 space-y-6 max-w-2xl">
      {/* Progress */}
      <div className="space-y-3">
        <div className="flex items-center justify-between">
          <h1 className="text-2xl font-bold">사전 등록</h1>
          <span className="text-sm text-muted-foreground">
            {completedSteps.length}/{steps.length}
          </span>
        </div>
        
        <Progress value={progress} className="h-2" />

        <div className="flex items-center justify-between">
          {steps.map((step) => {
            const Icon = step.icon;
            const isCompleted = completedSteps.includes(step.id);
            const isCurrent = currentStep === step.id;

            return (
              <button
                key={step.id}
                onClick={() => setCurrentStep(step.id)}
                className={`flex flex-col items-center gap-2 p-3 rounded-lg transition-smooth ${
                  isCurrent
                    ? "bg-primary/10 text-primary"
                    : isCompleted
                    ? "text-success"
                    : "text-muted-foreground"
                }`}
              >
                <div className={`relative ${isCompleted && "text-success"}`}>
                  <Icon className="w-6 h-6" />
                  {isCompleted && (
                    <Check className="absolute -right-1 -bottom-1 w-4 h-4 bg-success text-success-foreground rounded-full p-0.5" />
                  )}
                </div>
                <span className="text-xs font-medium">{step.label}</span>
              </button>
            );
          })}
        </div>
      </div>

      {/* Selfie Step */}
      {currentStep === "selfie" && (
        <Card className="border-border/50 bg-gradient-card shadow-lg">
          <CardHeader>
            <CardTitle className="flex items-center gap-2">
              <Camera className="w-5 h-5 text-primary" />
              얼굴 등록
            </CardTitle>
            <CardDescription>
              차량의 SCA가 본인 확인에 사용합니다
            </CardDescription>
          </CardHeader>
          <CardContent className="space-y-4">
            <div className="aspect-video bg-secondary/30 rounded-lg flex items-center justify-center border-2 border-dashed border-border">
              <div className="text-center space-y-2">
                <Camera className="w-12 h-12 mx-auto text-muted-foreground" />
                <p className="text-sm text-muted-foreground">카메라 미리보기</p>
              </div>
            </div>

            <div className="space-y-2 p-4 rounded-lg bg-primary/10 border border-primary/20">
              <h4 className="font-medium text-sm">촬영 가이드</h4>
              <ul className="text-xs text-muted-foreground space-y-1 list-disc list-inside">
                <li>정면을 바라봐 주세요</li>
                <li>조명이 밝은 곳에서 촬영하세요</li>
                <li>안경이나 모자는 벗어주세요</li>
              </ul>
            </div>

            <div className="flex gap-3">
              <Button variant="outline" className="flex-1">
                다시 찍기
              </Button>
              <Button variant="hero" className="flex-1" onClick={() => handleStepComplete("selfie")}>
                확인
              </Button>
            </div>
          </CardContent>
        </Card>
      )}

      {/* Body Metrics Step */}
      {currentStep === "body" && (
        <Card className="border-border/50 bg-gradient-card shadow-lg">
          <CardHeader>
            <CardTitle className="flex items-center gap-2">
              <Ruler className="w-5 h-5 text-primary" />
              체형 입력
            </CardTitle>
            <CardDescription>
              착좌 시 시트와 미러가 자동으로 조정됩니다
            </CardDescription>
          </CardHeader>
          <CardContent className="space-y-4">
            <div className="grid gap-4">
              <div className="space-y-2">
                <Label htmlFor="height">키 (cm)</Label>
                <Input
                  id="height"
                  type="number"
                  placeholder="176"
                  value={bodyMetrics.height}
                  onChange={(e) => setBodyMetrics({ ...bodyMetrics, height: e.target.value })}
                  className="bg-secondary/50 border-border"
                />
              </div>

              <div className="space-y-2">
                <Label htmlFor="sitHeight">앉은키 (cm)</Label>
                <Input
                  id="sitHeight"
                  type="number"
                  placeholder="92"
                  value={bodyMetrics.sitHeight}
                  onChange={(e) => setBodyMetrics({ ...bodyMetrics, sitHeight: e.target.value })}
                  className="bg-secondary/50 border-border"
                />
              </div>

              <div className="space-y-2">
                <Label htmlFor="shoulderWidth">어깨폭 (cm)</Label>
                <Input
                  id="shoulderWidth"
                  type="number"
                  placeholder="45"
                  value={bodyMetrics.shoulderWidth}
                  onChange={(e) => setBodyMetrics({ ...bodyMetrics, shoulderWidth: e.target.value })}
                  className="bg-secondary/50 border-border"
                />
              </div>

              <div className="space-y-2">
                <Label htmlFor="armLength">팔 길이 (cm)</Label>
                <Input
                  id="armLength"
                  type="number"
                  placeholder="60"
                  value={bodyMetrics.armLength}
                  onChange={(e) => setBodyMetrics({ ...bodyMetrics, armLength: e.target.value })}
                  className="bg-secondary/50 border-border"
                />
              </div>

              <div className="space-y-2">
                <Label htmlFor="viewAngle">시야각 (°)</Label>
                <Input
                  id="viewAngle"
                  type="number"
                  placeholder="12"
                  value={bodyMetrics.viewAngle}
                  onChange={(e) => setBodyMetrics({ ...bodyMetrics, viewAngle: e.target.value })}
                  className="bg-secondary/50 border-border"
                />
              </div>
            </div>

            <Button variant="hero" className="w-full" onClick={handleBodyMetricsSubmit}>
              다음 단계로
            </Button>
          </CardContent>
        </Card>
      )}

      {/* Device Step */}
      {currentStep === "device" && (
        <Card className="border-border/50 bg-gradient-card shadow-lg">
          <CardHeader>
            <CardTitle className="flex items-center gap-2">
              <Smartphone className="w-5 h-5 text-primary" />
              NFC/BT 페어링
            </CardTitle>
            <CardDescription>
              차량 탑승 시 SCA가 이 디바이스를 확인합니다
            </CardDescription>
          </CardHeader>
          <CardContent className="space-y-4">
            <div className="space-y-3">
              <div className="p-4 rounded-lg bg-secondary/30 space-y-2">
                <div className="flex items-center justify-between">
                  <span className="text-sm font-medium">NFC 지원</span>
                  <Check className="w-4 h-4 text-success" />
                </div>
                <div className="text-xs text-muted-foreground space-y-1">
                  <p>디바이스 ID: xxxxxx</p>
                  <p>공개키: 생성 완료</p>
                </div>
              </div>

              <div className="p-4 rounded-lg bg-secondary/30 space-y-2">
                <div className="flex items-center justify-between">
                  <span className="text-sm font-medium">블루투스</span>
                  <Check className="w-4 h-4 text-success" />
                </div>
                <div className="text-xs text-muted-foreground space-y-1">
                  <p>MAC: xx:xx:xx:xx</p>
                  <p>페어링 토큰: 생성 완료</p>
                </div>
              </div>
            </div>

            <div className="p-4 rounded-lg bg-primary/10 border border-primary/20">
              <p className="text-sm text-foreground">
                차량 탑승 시 SCA가 자동으로 이 디바이스를 확인합니다
              </p>
            </div>

            <Button variant="hero" className="w-full" onClick={() => handleStepComplete("device")}>
              서버에 등록
            </Button>
          </CardContent>
        </Card>
      )}
    </div>
  );
}
