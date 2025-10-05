#include "relay_module.hpp"

/** 내부 유틸: 논리 on/off → 실제 출력 레벨(0/1)로 변환 */
static inline int to_level( int active, bool on )
{
    // active==1: HIGH=ON → on?1:0
    // active==0:  LOW =ON → on?0:1
    return on ? active : !active;
}

bool RelayModule::begin( const gpio_num_t* pins,
                         size_t            count,
                         bool              active_high )
{
    pins_   = pins;
    count_  = count;
    active_ = active_high ? 1 : 0;

    // 1) 출력으로 쓸 핀들을 비트마스크로 합치기 (GPIO 드라이버 초기화용)
    uint64_t mask = 0ULL;
    for ( size_t i = 0; i < count_; ++i )
    {
        mask |= ( 1ULL << pins_[ i ] );
    }

    // 2) GPIO 설정 구조체 준비
    gpio_config_t io = {};
    io.pin_bit_mask  = mask;                    // 설정 대상 핀들의 집합
    io.mode          = GPIO_MODE_OUTPUT;        // 출력 모드
    io.pull_up_en    = GPIO_PULLUP_DISABLE;     // 내부 풀업 저항 사용 안 함
    io.pull_down_en  = GPIO_PULLDOWN_DISABLE;   // 내부 풀다운 저항 사용 안 함
    io.intr_type     = GPIO_INTR_DISABLE;       // 인터럽트 사용 안 함

    // 3) 설정 적용
    gpio_config( &io );

    // 4) 안전을 위해 전 채널 OFF로 시작
    for ( size_t i = 0; i < count_; ++i )
    {
        gpio_set_level( pins_[ i ], to_level( active_, /*on=*/false ) );
    }

    return true;
}

void RelayModule::set( size_t ch, bool on )
{
    if ( ch >= count_ ) return;

    const int level = to_level( active_, on );
    gpio_set_level( pins_[ ch ], level );
}

bool RelayModule::get( size_t ch ) const
{
    if ( ch >= count_ ) return false;

    const int level = gpio_get_level( pins_[ ch ] );
    return ( level != 0 );
}

void RelayModule::allOff()
{
    for ( size_t i = 0; i < count_; ++i )
    {
        gpio_set_level( pins_[ i ], to_level( active_, /*on=*/false ) );
    }
}
