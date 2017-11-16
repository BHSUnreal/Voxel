#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "TransitionDirection.h"

#define CHUNKSIZE_FC 18

class FValueOctree;
class FVoxelData;
struct FVoxelMaterial;

class FVoxelPolygonizerForCollisions
{
public:
	FVoxelPolygonizerForCollisions(FVoxelData* Data, FIntVector ChunkPosition);

	void CreateSection(FProcMeshSection& OutSection);

private:
	FVoxelData* const Data;
	FIntVector const ChunkPosition;

	FValueOctree* LastOctree;

	// Cache of the sign of the values. Can lead to crash if value changed between cache and 2nd access
	uint64 CachedSigns[216];

	// Cache to get index of already created vertices
	int Cache[CHUNKSIZE_FC][CHUNKSIZE_FC][CHUNKSIZE_FC][3]; // [SizeX][SizeY][SizeZ][3];;

	float CachedValues[(CHUNKSIZE_FC + 1) * (CHUNKSIZE_FC + 1) * (CHUNKSIZE_FC + 1)];

	FORCEINLINE float GetValue(int X, int Y, int Z);

	FORCEINLINE void SaveVertex(int X, int Y, int Z, short EdgeIndex, int Index);
	FORCEINLINE int LoadVertex(int X, int Y, int Z, short Direction, short EdgeIndex);
};