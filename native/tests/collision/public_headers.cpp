#include "occlusion/collision/collision.hpp"
#include <type_traits>
static_assert(std::is_copy_constructible_v<occlusion::collision::CollisionModel>);
