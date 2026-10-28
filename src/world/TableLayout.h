#pragma once

namespace fl::layout {
// World-unit dimensions shared by room geometry, previews and future cards.
// This is a capacity reference, not a mandatory grid or a gameplay card limit.
inline constexpr int columns=20,rows=20;
inline constexpr float cardWidth=0.28F,cardDepth=0.40F,cardThickness=0.012F;
inline constexpr float pitchX=0.32F,pitchZ=0.46F;
inline constexpr float feltWidth=7.2F,feltDepth=10.0F;
inline constexpr float tableWidth=7.6F,tableDepth=10.4F;
inline constexpr float cardCenterY=1.35F+cardThickness/2;
inline constexpr float roomHalfWidth=10.0F,roomHalfDepth=10.0F;
inline constexpr float seatZ=6.2F,approachZ=7.3F,spawnZ=8.45F;
inline constexpr float tableCameraDistance=18.5F,tableCameraPitch=1.25F;
inline constexpr float gridX(int column) {
    return (static_cast<float>(column)-static_cast<float>(columns-1)/2)*pitchX;
}
inline constexpr float gridZ(int row) {
    return (static_cast<float>(row)-static_cast<float>(rows-1)/2)*pitchZ;
}
static_assert(columns*pitchX <= feltWidth && rows*pitchZ <= feltDepth);
static_assert(cardWidth<pitchX && cardDepth<pitchZ);
}
