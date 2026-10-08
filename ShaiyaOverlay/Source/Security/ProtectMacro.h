#pragma once
#include <Windows.h>
#include <time.h>
#include <string>
//#include "CodeVirtualizer/include/VirtualizerSDK.h"
#include "skCrpyt.h"

//#pragma comment(lib, "Security/CodeVirtualizer/libs/COFF/VirtualizerSDK64.lib" )

#ifdef TEST_MODE

#define MProtectStart2( val )       //VIRTUALIZER_TIGER_RED_START
#define MProtectEnd2( )             //VIRTUALIZER_TIGER_RED_END

#define MProtectStart( val )        //VIRTUALIZER_TIGER_BLACK_START
#define MProtectEnd()               //VIRTUALIZER_TIGER_BLACK_END

#define pstra( val )                ( skCrypt( val ).decrypt() )
#define pstra8( val )               ( skCrypt( val ).decrypt() )
#define pstrw( val )                ( skCrypt( val ).decrypt() )


#define MProtectStartSpeed( val )	//VIRTUALIZER_TIGER_WHITE_START
#define MProtectEndSpeed( )		    //VIRTUALIZER_TIGER_WHITE_END

#else

#define MProtectStart2( val )       //VIRTUALIZER_TIGER_RED_START
#define MProtectEnd2( )             //VIRTUALIZER_TIGER_RED_END

#define MProtectStart( val )        //VIRTUALIZER_TIGER_BLACK_START
#define MProtectEnd()               //VIRTUALIZER_TIGER_BLACK_END

#define pstra( val )                ( skCrypt( val ).decrypt() )
#define pstra8( val )               ( skCrypt( val ).decrypt() )
#define pstrw( val )                ( skCrypt( val ).decrypt() )

#define MProtectStartSpeed( val )   //VIRTUALIZER_TIGER_WHITE_START
#define MProtectEndSpeed( )		    //VIRTUALIZER_TIGER_WHITE_END
#endif


#pragma optimize( "", off )
#pragma pack(push, 1)

struct SharedTheme
{
	char m_strName[50] = {};
	char m_strLink[200] = {};
	int nId = 0;

	struct sColors
	{
		std::uint32_t uText = 0;
		std::uint32_t uBg = 0;
		std::uint32_t uElementsPrimary = 0;
		std::uint32_t uElementsSecondary = 0;
	} Colors;

	std::size_t szImageSize = 0;
	std::uint8_t pImageData[ 8 ];
};

struct SharedInfo
{
	uint64_t	m_key = 0;

	struct sTimeInfo
	{
		int days = 0;
		int hours = 0;
		int min = 0;

	}m_end;

	tm m_LastAccess = { };

	char m_name[ 50 ] = { };
	uint32_t m_user_id = 0;
	uint32_t m_product_id = 0;
	uint64_t m_time_start = 0;
	uint64_t m_theme_ptr = 0;
	char m_pad[ 0x7E ] = { };

	FORCEINLINE void __stdcall RtlGetSystemTimeAsFileTime( LPFILETIME lpSystemTimeAsFileTime )
	{
		*lpSystemTimeAsFileTime = *reinterpret_cast<FILETIME*>( 0x7FFE0014 );
	}

	FORCEINLINE __time64_t __cdecl RtlTime64( __time64_t* Time )
	{
		FILETIME SystemTimeAsFileTime;

		RtlGetSystemTimeAsFileTime( &SystemTimeAsFileTime );

		auto result = ( *reinterpret_cast<ULONGLONG*>( &SystemTimeAsFileTime ) - 116444736000000000i64 ) / 10000000ui64;

		if ( result > 0x793406FFFi64 )
			result = -1i64;

		if ( Time )
			*Time = result;

		return result;
	}

	FORCEINLINE ULONGLONG __stdcall RtlGetTickCount64( )
	{
		return static_cast<ULONGLONG>( *reinterpret_cast<PULONG64>( 0x7FFE0000 + 0x320 ) * *reinterpret_cast<PULONG>( 0x7FFE0000 + 0x4 ) >> 24 );
	}

	FORCEINLINE void DecEnc( )
	{
		MProtectStart( "DecEnc" );

		tm ltm;

		auto now = this->RtlTime64( nullptr );

		auto er = _localtime64_s( &ltm, &now );

		if ( er )
			return;

		for ( size_t i = 0; i < sizeof( SharedInfo ); i++ )
			reinterpret_cast<uint8_t*>( this )[ i ] =
			reinterpret_cast<uint8_t*>( this )[ i ] ^ uint8_t( ltm.tm_yday ) + uint8_t( i * i );

		MProtectEnd( );

	}

	FORCEINLINE char* GetUser( )
	{
		char* buffer = new char[ sizeof( this->m_name ) ];

		memset( buffer, 0, sizeof( this->m_name ) );

		memcpy( buffer, this->m_name, sizeof( this->m_name ) );

		return buffer;
	}

	FORCEINLINE char* GetLastAccessStr( char* buffer )
	{
		memset( buffer, 0, 100 );

		sprintf_s( buffer, 100, pstra( "%02d:%02d hrs, %02d/%02d" ),
			m_LastAccess.tm_hour, m_LastAccess.tm_min, m_LastAccess.tm_mday, m_LastAccess.tm_mon + 1 );

		return buffer;
	}

	FORCEINLINE char* GetLastAccessStr( )
	{
		char* buffer = new char[ 100 ];

		return GetLastAccessStr( buffer );
	}

	FORCEINLINE char* GetEndAccessStr( char* buffer )
	{
		memset( buffer, 0, 100 );

		sprintf_s( buffer, 100, pstra( "%d days, %d hours, %d min" ), m_end.days, m_end.hours, m_end.min );

		buffer[ 99 ] = '\0';

		return buffer;
	}

	FORCEINLINE char* GetEndAccessStr( )
	{
		char* buffer = new char[ 100 ];

		return GetEndAccessStr( buffer );
	}

	FORCEINLINE ULONGLONG TickEnd( )
	{
		MProtectStart( "TickEnd" );

		auto TickNow = this->RtlGetTickCount64( );

		auto TickEnd =
			( TickNow +
				( 1000i64 * 60 * 60 * 24 * this->m_end.days ) +
				( 1000i64 * 60 * 60 * this->m_end.hours ) +
				( 1000i64 * 60 * this->m_end.min ) );

		MProtectEnd( );

		return TickEnd;
	}

	FORCEINLINE void CheckTick( )
	{
		ULONGLONG m_TickEnd = this->TickEnd( );

		MProtectStartSpeed( "CheckTick" );

		auto TickNow = this->RtlGetTickCount64( );

		static ULONGLONG TickInterval = 0;

		if ( TickNow > TickInterval )
		{
			TickInterval = TickNow + 5000;

			if ( TickNow > m_TickEnd )
				return abort( );

		}

		MProtectEndSpeed( );
	}

};
inline SharedInfo s_info;
#pragma pack(pop)
#pragma optimize( "", on )