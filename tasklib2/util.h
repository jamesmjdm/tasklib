#ifndef _UTIL_H
#define _UTIL_H

#include <optional>
#include <string>

// erase-remove idiom for removing from a container type in-place
// shorter to type and easier to remember
// do not call while iterating over the container!!
template<typename Container, typename Type>
void eraseRemove(Container& container, const Type& ty) {
	container.erase(remove(container.begin(), container.end(), ty), container.end());
}

std::optional<double> parseDouble(const std::string& str);

#endif
