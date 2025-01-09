#ifndef YY_THUNKS_IMPL_H
#define YY_THUNKS_IMPL_H

/*
 * copied from https://github.com/Chuyu-Team/YY-Thunks
 * MIT License
 * Copyright (c) 2018 Chuyu-Team
 */


// Implements wcsncpmp for ASCII chars only.
// NOTE: We can't use wcsncmp in this context because we may end up trying to modify
// locale data structs or even calling the same function in NLS code.
template<typename Char1, typename Char2>
static int _fastcall StringCompareIgnoreCaseByAscii(const Char1* string1, const Char2* string2, size_t count) noexcept
{
    wchar_t f, l;
    int result = 0;

    if (count)
    {
        /* validation section */
        do {
            f = __ascii_towlower(*string1);
            l = __ascii_towlower(*string2);
            string1++;
            string2++;
        } while ((--count) && f && (f == l));

        result = (int)(f - l);
    }

    return result;
}

static LPCWSTR __fastcall DownlevelNeutralToSpecificLocaleName(LPCWSTR szLocaleName)
{
    struct NeutralToSpecific
    {
        LPCWSTR szNeutralLocale;
        LPCWSTR szSpecificLocale;
    };


    static constexpr NeutralToSpecific NeutralToSpecificMap[] =
    {
        { L"af", L"af-ZA" },
        { L"am", L"am-ET" },
        { L"ar", L"ar-SA" },
        { L"arn", L"arn-CL" },
        { L"as", L"as-IN" },
        { L"az", L"az-Latn-AZ" },
        { L"az-Cyrl", L"az-Cyrl-AZ" },
        { L"az-Latn", L"az-Latn-AZ" },
        { L"ba", L"ba-RU" },
        { L"be", L"be-BY" },
        { L"bg", L"bg-BG" },
        { L"bn", L"bn-IN" },
        { L"bo", L"bo-CN" },
        { L"br", L"br-FR" },
        { L"bs-Cyrl", L"bs-Cyrl-BA" },
        { L"bs-Latn", L"bs-Latn-BA" },
        { L"ca", L"ca-ES" },
        { L"co", L"co-FR" },
        { L"cs", L"cs-CZ" },
        { L"cy", L"cy-GB" },
        { L"da", L"da-DK" },
        { L"de", L"de-DE" },
        { L"dsb", L"dsb-DE" },
        { L"dv", L"dv-MV" },
        { L"el", L"el-GR" },
        { L"en", L"en-US" },
        { L"es", L"es-ES" },
        { L"et", L"et-EE" },
        { L"eu", L"eu-ES" },
        { L"fa", L"fa-IR" },
        { L"fi", L"fi-FI" },
        { L"fil", L"fil-PH" },
        { L"fo", L"fo-FO" },
        { L"fr", L"fr-FR" },
        { L"fy", L"fy-NL" },
        { L"ga", L"ga-IE" },
        { L"gd", L"gd-GB" },
        { L"gl", L"gl-ES" },
        { L"gsw", L"gsw-FR" },
        { L"gu", L"gu-IN" },
        { L"ha-Latn", L"ha-Latn-NG" },
        { L"he", L"he-IL" },
        { L"hi", L"hi-IN" },
        { L"hr", L"hr-HR" },
        { L"hsb", L"hsb-DE" },
        { L"hu", L"hu-HU" },
        { L"hy", L"hy-AM" },
        { L"id", L"id-ID" },
        { L"ig", L"ig-NG" },
        { L"ii", L"ii-CN" },
        { L"is", L"is-IS" },
        { L"it", L"it-IT" },
        { L"iu-Cans", L"iu-Cans-CA" },
        { L"iu-Latn", L"iu-Latn-CA" },
        { L"ja", L"ja-JP" },
        { L"ka", L"ka-GE" },
        { L"kk", L"kk-KZ" },
        { L"kl", L"kl-GL" },
        { L"km", L"km-KH" },
        { L"kn", L"kn-IN" },
        { L"ko", L"ko-KR" },
        { L"kok", L"kok-IN" },
        { L"ky", L"ky-KG" },
        { L"lb", L"lb-LU" },
        { L"lo", L"lo-LA" },
        { L"lt", L"lt-LT" },
        { L"lv", L"lv-LV" },
        { L"mi", L"mi-NZ" },
        { L"mk", L"mk-MK" },
        { L"ml", L"ml-IN" },
        { L"mn", L"mn-MN" },
        { L"mn-Cyrl", L"mn-MN" },
        { L"mn-Mong", L"mn-Mong-CN" },
        { L"moh", L"moh-CA" },
        { L"mr", L"mr-IN" },
        { L"ms", L"ms-MY" },
        { L"mt", L"mt-MT" },
        { L"nb", L"nb-NO" },
        { L"ne", L"ne-NP" },
        { L"nl", L"nl-NL" },
        { L"nn", L"nn-NO" },
        { L"no", L"nb-NO" },
        { L"nso", L"nso-ZA" },
        { L"oc", L"oc-FR" },
        { L"or", L"or-IN" },
        { L"pa", L"pa-IN" },
        { L"pl", L"pl-PL" },
        { L"prs", L"prs-AF" },
        { L"ps", L"ps-AF" },
        { L"pt", L"pt-BR" },
        { L"qut", L"qut-GT" },
        { L"quz", L"quz-BO" },
        { L"rm", L"rm-CH" },
        { L"ro", L"ro-RO" },
        { L"ru", L"ru-RU" },
        { L"rw", L"rw-RW" },
        { L"sa", L"sa-IN" },
        { L"sah", L"sah-RU" },
        { L"se", L"se-NO" },
        { L"si", L"si-LK" },
        { L"sk", L"sk-SK" },
        { L"sl", L"sl-SI" },
        { L"sma", L"sma-SE" },
        { L"smj", L"smj-SE" },
        { L"smn", L"smn-FI" },
        { L"sms", L"sms-FI" },
        { L"sq", L"sq-AL" },
        { L"sr", L"sr-Latn-RS" },
        { L"sr-Cyrl", L"sr-Cyrl-RS" },
        { L"sr-Latn", L"sr-Latn-RS" },
        { L"sv", L"sv-SE" },
        { L"sw", L"sw-KE" },
        { L"syr", L"syr-SY" },
        { L"ta", L"ta-IN" },
        { L"te", L"te-IN" },
        { L"tg-Cyrl", L"tg-Cyrl-TJ" },
        { L"th", L"th-TH" },
        { L"tk", L"tk-TM" },
        { L"tn", L"tn-ZA" },
        { L"tr", L"tr-TR" },
        { L"tt", L"tt-RU" },
        { L"tzm-Latn", L"tzm-Latn-DZ" },
        { L"ug", L"ug-CN" },
        { L"uk", L"uk-UA" },
        { L"ur", L"ur-PK" },
        { L"uz", L"uz-Latn-UZ" },
        { L"uz-Cyrl", L"uz-Cyrl-UZ" },
        { L"uz-Latn", L"uz-Latn-UZ" },
        { L"vi", L"vi-VN" },
        { L"wo", L"wo-SN" },
        { L"xh", L"xh-ZA" },
        { L"yo", L"yo-NG" },
        { L"zh", L"zh-CN" },
        { L"zh-CHS", L"zh-CN" },
        { L"zh-CHT", L"zh-HK" },
        { L"zh-Hans", L"zh-CN" },
        { L"zh-Hant", L"zh-HK" },
        { L"zu", L"zu-ZA" },
    };


    int bottom = 0;
    int top = _countof(NeutralToSpecificMap) - 1;

    while (bottom <= top)
    {
        int middle = (bottom + top) / 2;
        int testIndex = StringCompareIgnoreCaseByAscii(szLocaleName, NeutralToSpecificMap[middle].szNeutralLocale, LOCALE_NAME_MAX_LENGTH);

        if (testIndex == 0)
            return NeutralToSpecificMap[middle].szSpecificLocale;

        if (testIndex < 0)
            top = middle - 1;
        else
            bottom = middle + 1;
    }

    //找不到就直接返回本身
    return szLocaleName;
}

static LCID WINAPI local_LocaleNameToLCID(LPCWSTR lpName, DWORD dwFlags)
{
   if (lpName == nullptr)
    {
        return GetUserDefaultLCID();
    }
    else if (StringCompareIgnoreCaseByAscii(lpName, LOCALE_NAME_SYSTEM_DEFAULT, _countof(LOCALE_NAME_SYSTEM_DEFAULT)) == 0)
    {
        return GetSystemDefaultLCID();
    }

    //Vista虽然不支持 LOCALE_ALLOW_NEUTRAL_NAMES，但是我们好人做到底，就直接支持了吧。
    if ((LOCALE_ALLOW_NEUTRAL_NAMES & dwFlags) == 0)
    {
        lpName = DownlevelNeutralToSpecificLocaleName(lpName);
    }

    // Map of locale name to LCID.
    struct LocaleNameToLcid
    {
        const wchar_t*  localeName;
        LCID            lcid;
    };


    // Map of locale name to an index in LcidToLocaleNameTable, for Windows XP.
    // Data in this table has been obtained from National Language Support (NLS) API Reference at
    // http://msdn.microsoft.com/en-us/goglobal/bb896001.aspx
    // The table is sorted to improve search performance.
    static const LocaleNameToLcid LocaleNameToLcidTable[] =
    {
        { L"", 0x0000007F },
        { L"af", 0x00000036 },
        { L"af-ZA", 0x00000436 },
        { L"am", 0x0000005E },
        { L"am-ET", 0x0000045E },
        { L"ar", 0x00000001 },
        { L"ar-AE", 0x00003801 },
        { L"ar-BH", 0x00003C01 },
        { L"ar-DZ", 0x00001401 },
        { L"ar-EG", 0x00000C01 },
        { L"ar-IQ", 0x00000801 },
        { L"ar-JO", 0x00002C01 },
        { L"ar-KW", 0x00003401 },
        { L"ar-LB", 0x00003001 },
        { L"ar-LY", 0x00001001 },
        { L"ar-MA", 0x00001801 },
        { L"ar-OM", 0x00002001 },
        { L"ar-QA", 0x00004001 },
        { L"ar-SA", 0x00000401 },
        { L"ar-SY", 0x00002801 },
        { L"ar-TN", 0x00001C01 },
        { L"ar-YE", 0x00002401 },
        { L"arn", 0x0000007A },
        { L"arn-CL", 0x0000047A },
        { L"as", 0x0000004D },
        { L"as-IN", 0x0000044D },
        { L"az", 0x0000002C },
        { L"az-Cyrl", 0x0000742C },
        { L"az-Cyrl-AZ", 0x0000082C },
        { L"az-Latn", 0x0000782C },
        { L"az-Latn-AZ", 0x0000042C },
        { L"ba", 0x0000006D },
        { L"ba-RU", 0x0000046D },
        { L"be", 0x00000023 },
        { L"be-BY", 0x00000423 },
        { L"bg", 0x00000002 },
        { L"bg-BG", 0x00000402 },
        { L"bn", 0x00000045 },
        { L"bn-BD", 0x00000845 },
        { L"bn-IN", 0x00000445 },
        { L"bo", 0x00000051 },
        { L"bo-CN", 0x00000451 },
        { L"br", 0x0000007E },
        { L"br-FR", 0x0000047E },
        { L"bs-Cyrl", 0x0000641A },
        { L"bs-Cyrl-BA", 0x0000201A },
        { L"bs-Latn", 0x0000681A },
        { L"bs-Latn-BA", 0x0000141A },
        { L"ca", 0x00000003 },
        { L"ca-ES", 0x00000403 },
        { L"co", 0x00000083 },
        { L"co-FR", 0x00000483 },
        { L"cs", 0x00000005 },
        { L"cs-CZ", 0x00000405 },
        { L"cy", 0x00000052 },
        { L"cy-GB", 0x00000452 },
        { L"da", 0x00000006 },
        { L"da-DK", 0x00000406 },
        { L"de", 0x00000007 },
        { L"de-AT", 0x00000C07 },
        { L"de-CH", 0x00000807 },
        { L"de-DE", 0x00000407 },
        { L"de-DE_phoneb", 0x00010407 },
        { L"de-LI", 0x00001407 },
        { L"de-LU", 0x00001007 },
        { L"dsb", 0x00007C2E },
        { L"dsb-DE", 0x0000082E },
        { L"dv", 0x00000065 },
        { L"dv-MV", 0x00000465 },
        { L"el", 0x00000008 },
        { L"el-GR", 0x00000408 },
        { L"en", 0x00000009 },
        { L"en-029", 0x00002409 },
        { L"en-AU", 0x00000C09 },
        { L"en-BZ", 0x00002809 },
        { L"en-CA", 0x00001009 },
        { L"en-GB", 0x00000809 },
        { L"en-IE", 0x00001809 },
        { L"en-IN", 0x00004009 },
        { L"en-JM", 0x00002009 },
        { L"en-MY", 0x00004409 },
        { L"en-NZ", 0x00001409 },
        { L"en-PH", 0x00003409 },
        { L"en-SG", 0x00004809 },
        { L"en-TT", 0x00002C09 },
        { L"en-US", 0x00000409 },
        { L"en-ZA", 0x00001C09 },
        { L"en-ZW", 0x00003009 },
        { L"es", 0x0000000A },
        { L"es-AR", 0x00002C0A },
        { L"es-BO", 0x0000400A },
        { L"es-CL", 0x0000340A },
        { L"es-CO", 0x0000240A },
        { L"es-CR", 0x0000140A },
        { L"es-DO", 0x00001C0A },
        { L"es-EC", 0x0000300A },
        { L"es-ES", 0x00000C0A },
        { L"es-ES_tradnl", 0x0000040A },
        { L"es-GT", 0x0000100A },
        { L"es-HN", 0x0000480A },
        { L"es-MX", 0x0000080A },
        { L"es-NI", 0x00004C0A },
        { L"es-PA", 0x0000180A },
        { L"es-PE", 0x0000280A },
        { L"es-PR", 0x0000500A },
        { L"es-PY", 0x00003C0A },
        { L"es-SV", 0x0000440A },
        { L"es-US", 0x0000540A },
        { L"es-UY", 0x0000380A },
        { L"es-VE", 0x0000200A },
        { L"et", 0x00000025 },
        { L"et-EE", 0x00000425 },
        { L"eu", 0x0000002D },
        { L"eu-ES", 0x0000042D },
        { L"fa", 0x00000029 },
        { L"fa-IR", 0x00000429 },
        { L"fi", 0x0000000B },
        { L"fi-FI", 0x0000040B },
        { L"fil", 0x00000064 },
        { L"fil-PH", 0x00000464 },
        { L"fo", 0x00000038 },
        { L"fo-FO", 0x00000438 },
        { L"fr", 0x0000000C },
        { L"fr-BE", 0x0000080C },
        { L"fr-CA", 0x00000C0C },
        { L"fr-CH", 0x0000100C },
        { L"fr-FR", 0x0000040C },
        { L"fr-LU", 0x0000140C },
        { L"fr-MC", 0x0000180C },
        { L"fy", 0x00000062 },
        { L"fy-NL", 0x00000462 },
        { L"ga", 0x0000003C },
        { L"ga-IE", 0x0000083C },
        { L"gd", 0x00000091 },
        { L"gd-GB", 0x00000491 },
        { L"gl", 0x00000056 },
        { L"gl-ES", 0x00000456 },
        { L"gsw", 0x00000084 },
        { L"gsw-FR", 0x00000484 },
        { L"gu", 0x00000047 },
        { L"gu-IN", 0x00000447 },
        { L"ha-Latn", 0x00007C68 },
        { L"ha-Latn-NG", 0x00000468 },
        { L"he", 0x0000000D },
        { L"he-IL", 0x0000040D },
        { L"hi", 0x00000039 },
        { L"hi-IN", 0x00000439 },
        { L"hr", 0x0000001A },
        { L"hr-BA", 0x0000101A },
        { L"hr-HR", 0x0000041A },
        { L"hsb", 0x0000002E },
        { L"hsb-DE", 0x0000042E },
        { L"hu", 0x0000000E },
        { L"hu-HU", 0x0000040E },
        { L"hu-HU_technl", 0x0001040E },
        { L"hy", 0x0000002B },
        { L"hy-AM", 0x0000042B },
        { L"id", 0x00000021 },
        { L"id-ID", 0x00000421 },
        { L"ig", 0x00000070 },
        { L"ig-NG", 0x00000470 },
        { L"ii", 0x00000078 },
        { L"ii-CN", 0x00000478 },
        { L"is", 0x0000000F },
        { L"is-IS", 0x0000040F },
        { L"it", 0x00000010 },
        { L"it-CH", 0x00000810 },
        { L"it-IT", 0x00000410 },
        { L"iu-Cans", 0x0000785D },
        { L"iu-Cans-CA", 0x0000045D },
        { L"iu-Latn", 0x00007C5D },
        { L"iu-Latn-CA", 0x0000085D },
        { L"ja", 0x00000011 },
        { L"ja-JP", 0x00000411 },
        { L"ja-JP_radstr", 0x00040411 },
        { L"ka", 0x00000037 },
        { L"ka-GE", 0x00000437 },
        { L"ka-GE_modern", 0x00010437 },
        { L"kk", 0x0000003F },
        { L"kk-KZ", 0x0000043F },
        { L"kl", 0x0000006F },
        { L"kl-GL", 0x0000046F },
        { L"km", 0x00000053 },
        { L"km-KH", 0x00000453 },
        { L"kn", 0x0000004B },
        { L"kn-IN", 0x0000044B },
        { L"ko", 0x00000012 },
        { L"ko-KR", 0x00000412 },
        { L"kok", 0x00000057 },
        { L"kok-IN", 0x00000457 },
        { L"ky", 0x00000040 },
        { L"ky-KG", 0x00000440 },
        { L"lb", 0x0000006E },
        { L"lb-LU", 0x0000046E },
        { L"lo", 0x00000054 },
        { L"lo-LA", 0x00000454 },
        { L"lt", 0x00000027 },
        { L"lt-LT", 0x00000427 },
        { L"lv", 0x00000026 },
        { L"lv-LV", 0x00000426 },
        { L"mi", 0x00000081 },
        { L"mi-NZ", 0x00000481 },
        { L"mk", 0x0000002F },
        { L"mk-MK", 0x0000042F },
        { L"ml", 0x0000004C },
        { L"ml-IN", 0x0000044C },
        { L"mn", 0x00000050 },
        { L"mn-Cyrl", 0x00007850 },
        { L"mn-MN", 0x00000450 },
        { L"mn-Mong", 0x00007C50 },
        { L"mn-Mong-CN", 0x00000850 },
        { L"moh", 0x0000007C },
        { L"moh-CA", 0x0000047C },
        { L"mr", 0x0000004E },
        { L"mr-IN", 0x0000044E },
        { L"ms", 0x0000003E },
        { L"ms-BN", 0x0000083E },
        { L"ms-MY", 0x0000043E },
        { L"mt", 0x0000003A },
        { L"mt-MT", 0x0000043A },
        { L"nb", 0x00007C14 },
        { L"nb-NO", 0x00000414 },
        { L"ne", 0x00000061 },
        { L"ne-NP", 0x00000461 },
        { L"nl", 0x00000013 },
        { L"nl-BE", 0x00000813 },
        { L"nl-NL", 0x00000413 },
        { L"nn", 0x00007814 },
        { L"nn-NO", 0x00000814 },
        { L"no", 0x00000014 },
        { L"nso", 0x0000006C },
        { L"nso-ZA", 0x0000046C },
        { L"oc", 0x00000082 },
        { L"oc-FR", 0x00000482 },
        { L"or", 0x00000048 },
        { L"or-IN", 0x00000448 },
        { L"pa", 0x00000046 },
        { L"pa-IN", 0x00000446 },
        { L"pl", 0x00000015 },
        { L"pl-PL", 0x00000415 },
        { L"prs", 0x0000008C },
        { L"prs-AF", 0x0000048C },
        { L"ps", 0x00000063 },
        { L"ps-AF", 0x00000463 },
        { L"pt", 0x00000016 },
        { L"pt-BR", 0x00000416 },
        { L"pt-PT", 0x00000816 },
        { L"qps-ploc", 0x00000501 },
        { L"qps-ploca", 0x000005FE },
        { L"qps-plocm", 0x000009FF },
        { L"qut", 0x00000086 },
        { L"qut-GT", 0x00000486 },
        { L"quz", 0x0000006B },
        { L"quz-BO", 0x0000046B },
        { L"quz-EC", 0x0000086B },
        { L"quz-PE", 0x00000C6B },
        { L"rm", 0x00000017 },
        { L"rm-CH", 0x00000417 },
        { L"ro", 0x00000018 },
        { L"ro-RO", 0x00000418 },
        { L"ru", 0x00000019 },
        { L"ru-RU", 0x00000419 },
        { L"rw", 0x00000087 },
        { L"rw-RW", 0x00000487 },
        { L"sa", 0x0000004F },
        { L"sa-IN", 0x0000044F },
        { L"sah", 0x00000085 },
        { L"sah-RU", 0x00000485 },
        { L"se", 0x0000003B },
        { L"se-FI", 0x00000C3B },
        { L"se-NO", 0x0000043B },
        { L"se-SE", 0x0000083B },
        { L"si", 0x0000005B },
        { L"si-LK", 0x0000045B },
        { L"sk", 0x0000001B },
        { L"sk-SK", 0x0000041B },
        { L"sl", 0x00000024 },
        { L"sl-SI", 0x00000424 },
        { L"sma", 0x0000783B },
        { L"sma-NO", 0x0000183B },
        { L"sma-SE", 0x00001C3B },
        { L"smj", 0x00007C3B },
        { L"smj-NO", 0x0000103B },
        { L"smj-SE", 0x0000143B },
        { L"smn", 0x0000703B },
        { L"smn-FI", 0x0000243B },
        { L"sms", 0x0000743B },
        { L"sms-FI", 0x0000203B },
        { L"sq", 0x0000001C },
        { L"sq-AL", 0x0000041C },
        { L"sr", 0x00007C1A },
        { L"sr-Cyrl", 0x00006C1A },
        { L"sr-Cyrl-BA", 0x00001C1A },
        { L"sr-Cyrl-CS", 0x00000C1A },
        { L"sr-Cyrl-ME", 0x0000301A },
        { L"sr-Cyrl-RS", 0x0000281A },
        { L"sr-Latn", 0x0000701A },
        { L"sr-Latn-BA", 0x0000181A },
        { L"sr-Latn-CS", 0x0000081A },
        { L"sr-Latn-ME", 0x00002C1A },
        { L"sr-Latn-RS", 0x0000241A },
        { L"sv", 0x0000001D },
        { L"sv-FI", 0x0000081D },
        { L"sv-SE", 0x0000041D },
        { L"sw", 0x00000041 },
        { L"sw-KE", 0x00000441 },
        { L"syr", 0x0000005A },
        { L"syr-SY", 0x0000045A },
        { L"ta", 0x00000049 },
        { L"ta-IN", 0x00000449 },
        { L"te", 0x0000004A },
        { L"te-IN", 0x0000044A },
        { L"tg-Cyrl", 0x00007C28 },
        { L"tg-Cyrl-TJ", 0x00000428 },
        { L"th", 0x0000001E },
        { L"th-TH", 0x0000041E },
        { L"tk", 0x00000042 },
        { L"tk-TM", 0x00000442 },
        { L"tn", 0x00000032 },
        { L"tn-ZA", 0x00000432 },
        { L"tr", 0x0000001F },
        { L"tr-TR", 0x0000041F },
        { L"tt", 0x00000044 },
        { L"tt-RU", 0x00000444 },
        { L"tzm-Latn", 0x00007C5F },
        { L"tzm-Latn-DZ", 0x0000085F },
        { L"ug", 0x00000080 },
        { L"ug-CN", 0x00000480 },
        { L"uk", 0x00000022 },
        { L"uk-UA", 0x00000422 },
        { L"ur", 0x00000020 },
        { L"ur-PK", 0x00000420 },
        { L"uz", 0x00000043 },
        { L"uz-Cyrl", 0x00007843 },
        { L"uz-Cyrl-UZ", 0x00000843 },
        { L"uz-Latn", 0x00007C43 },
        { L"uz-Latn-UZ", 0x00000443 },
        { L"vi", 0x0000002A },
        { L"vi-VN", 0x0000042A },
        { L"wo", 0x00000088 },
        { L"wo-SN", 0x00000488 },
        { L"x-IV_mathan", 0x0001007F },
        { L"xh", 0x00000034 },
        { L"xh-ZA", 0x00000434 },
        { L"yo", 0x0000006A },
        { L"yo-NG", 0x0000046A },
        { L"zh", 0x00007804 },
        { L"zh-CHS", 0x00000004 },
        { L"zh-CHT", 0x00007C04 },
        { L"zh-CN", 0x00000804 },
        { L"zh-CN_stroke", 0x00020804 },
        { L"zh-Hans", 0x00000004 },
        { L"zh-Hant", 0x00007C04 },
        { L"zh-HK", 0x00000C04 },
        { L"zh-HK_radstr", 0x00040C04 },
        { L"zh-MO", 0x00001404 },
        { L"zh-MO_radstr", 0x00041404 },
        { L"zh-MO_stroke", 0x00021404 },
        { L"zh-SG", 0x00001004 },
        { L"zh-SG_stroke", 0x00021004 },
        { L"zh-TW", 0x00000404 },
        { L"zh-TW_pronun", 0x00030404 },
        { L"zh-TW_radstr", 0x00040404 },
        { L"zu", 0x00000035 },
        { L"zu-ZA", 0x00000435 },
    };



    int bottom = 0;
    int top = _countof(LocaleNameToLcidTable) - 1;

    while (bottom <= top)
    {
        int middle = (bottom + top) / 2;
        int testIndex = StringCompareIgnoreCaseByAscii(lpName, LocaleNameToLcidTable[middle].localeName, LOCALE_NAME_MAX_LENGTH);

        if (testIndex == 0)
            return LocaleNameToLcidTable[middle].lcid;

        if (testIndex < 0)
            top = middle - 1;
        else
            bottom = middle + 1;
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
}

#endif //YY_THUNKS_IMPL_H
