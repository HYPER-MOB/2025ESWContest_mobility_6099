#pragma once
#include "driver/gpio.h"
#include <cstddef>
#include <cstdint>

/**
 * RelayModule
 * - 단일 코일 릴레이들을 ON/OFF 하는 가장 기본 모듈
 * - ULN2803A(싱크 드라이버) + 기계식 릴레이 코일 제어에 적합
 * - active_high = true  이면 GPIO=HIGH → 코일 ON (권장: ULN 입력은 HIGH=ON)
 * - active_high = false 이면 GPIO=LOW  → 코일 ON (액티브-로우 회로에 사용)
 */
class RelayModule
{
public:
    /** 하드웨어 초기화
     *  @param pins        GPIO 번호 배열 (esp-idf enum gpio_num_t)
     *  @param count       릴레이 채널 개수
     *  @param active_high 코일 ON을 만드는 논리 레벨 (기본 true)
     */
    bool begin( const gpio_num_t* pins,
                size_t            count,
                bool              active_high = true );

    /** 채널 on/off 제어
     *  @param ch  채널 인덱스 (0..count-1)
     *  @param on  true=ON, false=OFF
     */
    void set( size_t ch, bool on );

    /** 현재 물리 출력 상태 읽기 (active_high 기준이 아니라 실제 핀 레벨) */
    bool get( size_t ch ) const;

    /** 전 채널 OFF */
    void allOff();

    /** 채널 개수 반환 */
    size_t size() const { return count_; }

    /** 현재 구성(논리) 조회 */
    bool isActiveHigh() const { return active_ == 1; }

private:
    const gpio_num_t* pins_  { nullptr };  // 물리 핀 테이블(외부 소유)
    size_t            count_ { 0 };        // 채널 개수
    int               active_{ 1 };        // 1: HIGH=ON, 0: LOW=ON
};
