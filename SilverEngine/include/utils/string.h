#pragma once

namespace sv {

    // Character utils
    
    constexpr bool char_is_letter(char c) 
    {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }
    
    constexpr bool char_is_number(char c)
    {
	return c >= '0' && c <= '9';
    }
    
}
