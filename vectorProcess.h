#pragma once
#include <vector>
#include <string>

_EXPORT_STD template <class _InIt, class _Ty>
_NODISCARD _CONSTEXPR20 _InIt contains(std::vector<_InIt> Vec, const _Ty& _Val) 
{ // find first matching _Val
	std::find(Vec.begin(), Vec.end(), _Val);
}