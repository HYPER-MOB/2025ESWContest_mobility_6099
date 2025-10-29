import { useState } from "react";
import { Link } from "react-router-dom";
import { Search, MapPin, Settings2, ChevronRight } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Card, CardContent } from "@/components/ui/card";
import { Badge } from "@/components/ui/badge";

interface Vehicle {
  id: string;
  model: string;
  location: string;
  capabilities: string[];
  available: boolean;
}

const mockVehicles: Vehicle[] = [
  { id: "V-001", model: "SEDAN-24", location: "SEOUL-01", capabilities: ["seat", "mirror"], available: true },
  { id: "V-002", model: "SEDAN-25", location: "SEOUL-02", capabilities: ["seat", "mirror", "steering"], available: true },
  { id: "V-003", model: "SUV-10", location: "BUSAN-01", capabilities: ["seat", "mirror"], available: false },
  { id: "V-004", model: "HATCH-15", location: "SEOUL-03", capabilities: ["seat"], available: true },
];

export default function Rentals() {
  const [searchQuery, setSearchQuery] = useState("");
  const [showFilters, setShowFilters] = useState(false);

  const filteredVehicles = mockVehicles.filter((v) =>
    v.model.toLowerCase().includes(searchQuery.toLowerCase()) ||
    v.location.toLowerCase().includes(searchQuery.toLowerCase())
  );

  return (
    <div className="container mx-auto p-4 space-y-6 max-w-2xl">
      {/* Header */}
      <div className="space-y-2">
        <h1 className="text-2xl font-bold">차량 대여</h1>
        <p className="text-muted-foreground">원하는 차량을 선택하세요</p>
      </div>

      {/* Search & Filters */}
      <div className="space-y-3">
        <div className="relative">
          <Search className="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-muted-foreground" />
          <Input
            placeholder="지역 또는 차종 검색"
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            className="pl-10 bg-secondary/50 border-border"
          />
        </div>

        <Button
          variant="outline"
          className="w-full justify-start gap-2"
          onClick={() => setShowFilters(!showFilters)}
        >
          <Settings2 className="w-4 h-4" />
          필터 (좌석·미러·스티어링)
        </Button>
      </div>

      {/* Vehicle List */}
      <div className="space-y-3">
        {filteredVehicles.map((vehicle) => (
          <Link key={vehicle.id} to={`/rental/${vehicle.id}`}>
            <Card className="border-border/50 bg-gradient-card hover:border-primary/50 transition-smooth cursor-pointer group">
              <CardContent className="p-4">
                <div className="flex items-center justify-between">
                  <div className="flex-1 space-y-2">
                    <div className="flex items-center gap-3">
                      <h3 className="font-semibold text-lg group-hover:text-primary transition-smooth">
                        {vehicle.model}
                      </h3>
                      {!vehicle.available && (
                        <Badge variant="outline" className="bg-destructive/20 text-destructive border-destructive/30">
                          대여중
                        </Badge>
                      )}
                    </div>

                    <div className="flex items-center gap-2 text-sm text-muted-foreground">
                      <MapPin className="w-3.5 h-3.5" />
                      {vehicle.location}
                    </div>

                    <div className="flex gap-2">
                      {vehicle.capabilities.map((cap) => (
                        <Badge
                          key={cap}
                          variant="secondary"
                          className="bg-primary/10 text-primary border-primary/30"
                        >
                          {cap}
                        </Badge>
                      ))}
                    </div>
                  </div>

                  <ChevronRight className="w-5 h-5 text-muted-foreground group-hover:text-primary transition-smooth" />
                </div>
              </CardContent>
            </Card>
          </Link>
        ))}

        {filteredVehicles.length === 0 && (
          <Card className="border-border/50 bg-card/50">
            <CardContent className="p-8 text-center">
              <p className="text-muted-foreground">검색 결과가 없습니다</p>
            </CardContent>
          </Card>
        )}
      </div>
    </div>
  );
}
