import { cn } from "@/lib/utils";
import { CheckCircle2, Clock, XCircle, AlertCircle, LucideIcon } from "lucide-react";

export type RentalStatus = "requested" | "approved" | "picked" | "returned" | "canceled";
export type MFAStatus = "pending" | "smartkey_ok" | "face_ok" | "nfc_bt_ok" | "passed" | "failed";
export type SafetyStatus = "ok" | "block";

interface StatusBadgeProps {
  type: "rental" | "mfa" | "safety";
  status: RentalStatus | MFAStatus | SafetyStatus;
  className?: string;
}

interface StatusConfig {
  label: string;
  color: string;
  icon: LucideIcon;
}

const statusConfig: Record<string, Record<string, StatusConfig>> = {
  rental: {
    requested: { label: "요청됨", color: "bg-warning/20 text-warning border-warning/30", icon: Clock },
    approved: { label: "승인됨", color: "bg-success/20 text-success border-success/30", icon: CheckCircle2 },
    picked: { label: "수령됨", color: "bg-primary/20 text-primary border-primary/30", icon: CheckCircle2 },
    returned: { label: "반납됨", color: "bg-muted/50 text-muted-foreground border-muted", icon: CheckCircle2 },
    canceled: { label: "취소됨", color: "bg-destructive/20 text-destructive border-destructive/30", icon: XCircle },
  },
  mfa: {
    pending: { label: "대기중", color: "bg-muted/50 text-muted-foreground border-muted", icon: Clock },
    smartkey_ok: { label: "스마트키 ✓", color: "bg-primary/20 text-primary border-primary/30", icon: CheckCircle2 },
    face_ok: { label: "얼굴 인식 ✓", color: "bg-primary/20 text-primary border-primary/30", icon: CheckCircle2 },
    nfc_bt_ok: { label: "NFC/BT ✓", color: "bg-primary/20 text-primary border-primary/30", icon: CheckCircle2 },
    passed: { label: "인증 완료", color: "bg-success/20 text-success border-success/30", icon: CheckCircle2 },
    failed: { label: "인증 실패", color: "bg-destructive/20 text-destructive border-destructive/30", icon: XCircle },
  },
  safety: {
    ok: { label: "안전", color: "bg-success/20 text-success border-success/30", icon: CheckCircle2 },
    block: { label: "차단됨", color: "bg-destructive/20 text-destructive border-destructive/30", icon: AlertCircle },
  },
};

export default function StatusBadge({ type, status, className }: StatusBadgeProps) {
  const config = statusConfig[type]?.[status as string];
  if (!config) return null;
  
  const Icon = config.icon;

  return (
    <span
      className={cn(
        "inline-flex items-center gap-1.5 px-3 py-1 rounded-full text-xs font-medium border transition-smooth",
        config.color,
        className
      )}
    >
      <Icon className="w-3.5 h-3.5" />
      {config.label}
    </span>
  );
}
