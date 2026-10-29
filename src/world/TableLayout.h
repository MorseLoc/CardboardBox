#pragma once

namespace fl::layout {
// World-unit dimensions shared by room geometry, previews and future cards.
// This is a capacity reference, not a mandatory grid or a gameplay card limit.
inline constexpr int columns=20,rows=10;
inline constexpr float cardWidth=0.28F,cardDepth=0.40F,cardThickness=0.0015F;
inline constexpr float pitchX=0.32F,pitchZ=0.46F;
inline constexpr float feltWidth=7.2F,feltDepth=5.4F;
inline constexpr float tableWidth=7.6F,tableDepth=5.8F;
inline constexpr float cardCenterY=1.35F+cardThickness/2;
inline constexpr float roomHalfWidth=10.0F,roomHalfDepth=10.0F;
inline constexpr float seatZ=3.85F,approachZ=4.95F,spawnZ=6.2F;
inline constexpr float tableCameraDistance=12.8F,tableCameraPitch=1.12F;
inline constexpr float gridX(int column) {
    return (static_cast<float>(column)-static_cast<float>(columns-1)/2)*pitchX;
}
inline constexpr float gridZ(int row) {
    return (static_cast<float>(row)-static_cast<float>(rows-1)/2)*pitchZ;
}
static_assert(columns*pitchX <= feltWidth && rows*pitchZ <= feltDepth);
static_assert(cardWidth<pitchX && cardDepth<pitchZ);
}
