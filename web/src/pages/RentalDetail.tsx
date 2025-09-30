import { useParams, useNavigate } from "react-router-dom";
import { ArrowLeft, MapPin, Calendar, Clock, Car, Shield } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import StatusBadge from "@/components/StatusBadge";
import { useToast } from "@/hooks/use-toast";

export default function RentalDetail() {
  const { id } = useParams();
  const navigate = useNavigate();
  const { toast } = useToast();

  // Mock data
  const rental = {
    id: id || "R-001",
    vehicle: "SEDAN-24",
    location: "SEOUL-01",
    startTime: "10:00",
    endTime: "12:00",
    date: "2025-01-15",
    status: "approved" as const,
    timeline: [
      { time: "10:00", status: "requested", label: "예약 요청", actor: "사용자" },
      { time: "10:02", status: "approved", label: "예약 승인", actor: "관리자" },
      { time: "--", status: "picked", label: "차량 수령", actor: "" },
    ],
  };

  const handleCancel = () => {
    toast({
      title: "예약 취소",
      description: "예약이 취소되었습니다",
    });
    navigate("/rentals");
  };

  return (
    <div className="container mx-auto p-4 space-y-6 max-w-2xl">
      {/* Header */}
      <div className="flex items-center gap-3">
        <Button variant="ghost" size="icon" onClick={() => navigate(-1)}>
          <ArrowLeft className="w-5 h-5" />
        </Button>
        <div>
          <h1 className="text-2xl font-bold">예약 상세</h1>
          <p className="text-sm text-muted-foreground">예약 ID: {rental.id}</p>
        </div>
      </div>

      {/* Status */}
      <Card className="border-border/50 bg-gradient-card shadow-lg">
        <CardContent className="p-6">
          <div className="flex items-center justify-between mb-4">
            <span className="text-sm text-muted-foreground">현재 상태</span>
            <StatusBadge type="rental" status={rental.status} />
          </div>
          
          <div className="space-y-4">
            <div className="flex items-center gap-3">
              <Car className="w-5 h-5 text-primary" />
              <div>
                <p className="font-semibold text-lg">{rental.vehicle}</p>
                <div className="flex items-center gap-2 text-sm text-muted-foreground">
                  <MapPin className="w-3.5 h-3.5" />
                  {rental.location}
                </div>
              </div>
            </div>

            <div className="grid grid-cols-2 gap-4">
              <div className="flex items-center gap-2 text-sm">
                <Calendar className="w-4 h-4 text-primary" />
                <span>{rental.date}</span>
              </div>
              <div className="flex items-center gap-2 text-sm">
                <Clock className="w-4 h-4 text-primary" />
                <span>{rental.startTime}~{rental.endTime}</span>
              </div>
            </div>
          </div>
        </CardContent>
      </Card>

      {/* Timeline */}
      <Card className="border-border/50 bg-gradient-card shadow-lg">
        <CardHeader>
          <CardTitle>예약 진행 상황</CardTitle>
        </CardHeader>
        <CardContent>
          <div className="space-y-4">
            {rental.timeline.map((event, index) => (
              <div key={index} className="flex gap-4">
                <div className="flex flex-col items-center">
                  <div
                    className={`w-3 h-3 rounded-full ${
                      event.time !== "--" ? "bg-success" : "bg-muted"
                    }`}
                  />
                  {index < rental.timeline.length - 1 && (
                    <div className="w-0.5 h-12 bg-border" />
                  )}
                </div>
                <div className="flex-1 pb-4">
                  <div className="flex items-center justify-between mb-1">
                    <span className="font-medium">{event.label}</span>
                    <span className="text-xs text-muted-foreground">{event.time}</span>
                  </div>
                  {event.actor && (
                    <span className="text-xs text-muted-foreground">{event.actor}</span>
                  )}
                </div>
              </div>
            ))}
          </div>
        </CardContent>
      </Card>

      {/* MFA Notice */}
      <Card className="border-primary/50 bg-primary/5 shadow-lg">
        <CardContent className="p-6 space-y-3">
          <div className="flex items-center gap-2">
            <Shield className="w-5 h-5 text-primary" />
            <h3 className="font-semibold">안내사항</h3>
          </div>
          <p className="text-sm text-foreground">
            차량 탑승 시 자동으로 MFA 인증이 진행됩니다
          </p>
          <ul className="text-xs text-muted-foreground space-y-1 list-disc list-inside">
            <li>스마트키 확인 (차량)</li>
            <li>얼굴 인식 (SCA)</li>
            <li>NFC/BT 확인 (모바일)</li>
          </ul>
        </CardContent>
      </Card>

      {/* Actions */}
      {rental.status === "approved" && (
        <Button variant="outline" className="w-full" onClick={handleCancel}>
          예약 취소
        </Button>
      )}
    </div>
  );
}
