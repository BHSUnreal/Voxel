#include "ValueOctree.h"
#include "VoxelPrivatePCH.h"
#include "VoxelData.h"
#include "VoxelWorldGenerator.h"
#include "GenericPlatformProcess.h"

FValueOctree::FValueOctree(UVoxelWorldGenerator* WorldGenerator, FIntVector Position, uint8 Depth, uint64 Id, bool bMultiplayer)
	: FOctree(Position, Depth, Id)
	, WorldGenerator(WorldGenerator)
	, bIsDirty(false)
	, bIsNetworkDirty(false)
	, bMultiplayer(bMultiplayer)
{

}

FValueOctree::~FValueOctree()
{
	if (bHasChilds)
	{
		for (auto Child : Childs)
		{
			delete Child;
		}
	}
}

bool FValueOctree::IsDirty() const
{
	return bIsDirty;
}

void FValueOctree::GetValuesAndMaterials(float InValues[], FVoxelMaterial InMaterials[], const FIntVector& Start, const FIntVector& StartIndex, const int Step, const FIntVector& Size, const FIntVector& ArraySize) const
{
	check(IsInOctree(Start.X, Start.Y, Start.Z));
	check(IsInOctree(Start.X + Size.X, Start.Y + Size.Y, Start.Z + Size.Z));

	if (IsLeaf())
	{
		if (UNLIKELY(IsDirty()))
		{
			for (int X = 0; X < Size.X; X += Step)
			{
				for (int Y = 0; Y < Size.Y; Y += Step)
				{
					for (int Z = 0; Z < Size.Z; Z += Step)
					{
						int LocalX, LocalY, LocalZ;
						GlobalToLocal(Start.X + X, Start.Y + Y, Start.Z + Z, LocalX, LocalY, LocalZ);

						const int LocalIndex = IndexFromCoordinates(LocalX, LocalY, LocalZ);
						const int Index = (StartIndex.X + X) + ArraySize.X * (StartIndex.Y + Y) + ArraySize.X * ArraySize.Y * (StartIndex.Z + Z);

						if (InValues)
						{
							InValues[Index] = Values[LocalIndex];
						}
						if (InMaterials)
						{
							InMaterials[Index] = Materials[LocalIndex];
						}
					}
				}
			}
		}
		else
		{
			WorldGenerator->GetValuesAndMaterials(InValues, InMaterials, Start, StartIndex, Step, Size, ArraySize);
		}
	}
	else
	{
		const int StartI = StartIndex.X;
		const int StartJ = StartIndex.Y;
		const int StartK = StartIndex.Z;

		const int StartX = Start.X;
		const int StartY = Start.Y;
		const int StartZ = Start.Z;

		const int SizeX = Size.X;
		const int SizeY = Size.Y;
		const int SizeZ = Size.Z;

		const int StartXBot = StartX;
		const int StartXTop = Position.X;
		const int StartIBot = StartI;
		const int StartITop = StartI + (Position.X - StartX);
		const int SizeXBot = Position.X - StartX;
		const int SizeXTop = SizeX - (Position.X - StartX);

		const int StartYBot = StartY;
		const int StartYTop = Position.Y;
		const int StartJBot = StartJ;
		const int StartJTop = StartJ + (Position.Y - StartY);
		const int SizeYBot = Position.Y - StartY;
		const int SizeYTop = SizeY - (Position.Y - StartY);

		const int StartZBot = StartZ;
		const int StartZTop = Position.Z;
		const int StartKBot = StartK;
		const int StartKTop = StartK + (Position.Z - StartZ);
		const int SizeZBot = Position.Z - StartZ;
		const int SizeZTop = SizeZ - (Position.Z - StartZ);

		if (StartX + SizeX < Position.X || Position.X <= StartX)
		{
			if (StartY + SizeY < Position.Y || Position.Y <= StartY)
			{
				if (StartZ + SizeZ < Position.Z || Position.Z <= StartZ)
				{
					GetChild(StartX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartX, StartY, StartZ },
					{ StartI, StartJ, StartK },
						Step,
						{ SizeX, SizeY, SizeZ },
						ArraySize);
				}
				else
				{
					// Split Z

					GetChild(StartX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartX, StartY, StartZBot },
					{ StartI, StartJ, StartKBot },
						Step,
						{ SizeX, SizeY, SizeZBot },
						ArraySize);

					GetChild(StartX, StartY, StartZ + SizeZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartX, StartY, StartZTop },
					{ StartI, StartJ, StartKTop },
						Step,
						{ SizeX, SizeY, SizeZTop },
						ArraySize);
				}
			}
			else
			{
				// Split Y

				if (StartZ + SizeZ < Position.Z || Position.Z <= StartZ)
				{
					GetChild(StartX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartX, StartYBot, StartZ },
					{ StartI, StartJBot, StartK },
						Step,
						{ SizeX, SizeYBot, SizeZ },
						ArraySize);

					GetChild(StartX, StartY + SizeY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartX, StartYTop, StartZ },
					{ StartI, StartJTop, StartK },
						Step,
						{ SizeX, SizeYTop, SizeZ },
						ArraySize);
				}
				else
				{
					// Split Z

					GetChild(StartX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartX, StartYBot, StartZBot },
					{ StartI, StartJBot, StartKBot },
						Step,
						{ SizeX, SizeYBot, SizeZBot },
						ArraySize);

					GetChild(StartX, StartY, StartZ + SizeZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartX, StartYBot, StartZTop },
					{ StartI, StartJBot, StartKTop },
						Step,
						{ SizeX, SizeYBot, SizeZTop },
						ArraySize);


					GetChild(StartX, StartY + SizeY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartX, StartYTop, StartZBot },
					{ StartI, StartJTop, StartKBot },
						Step,
						{ SizeX, SizeYTop, SizeZBot },
						ArraySize);

					GetChild(StartX, StartY + SizeY, StartZ + SizeZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartX, StartYTop, StartZTop },
					{ StartI, StartJTop, StartKTop },
						Step,
						{ SizeX, SizeYTop, SizeZTop },
						ArraySize);
				}
			}
		}
		else
		{
			// Split X

			if (StartY + SizeY < Position.Y || Position.Y <= StartY)
			{
				if (StartZ + SizeZ < Position.Z || Position.Z <= StartZ)
				{
					GetChild(StartX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartXBot, StartY, StartZ },
					{ StartIBot, StartJ, StartK },
						Step,
						{ SizeXBot, SizeY, SizeZ },
						ArraySize);

					GetChild(StartX + SizeX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartXTop, StartY, StartZ },
					{ StartITop, StartJ, StartK },
						Step,
						{ SizeXTop, SizeY, SizeZ },
						ArraySize);
				}
				else
				{
					// Split Z

					GetChild(StartX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartXBot, StartY, StartZBot },
					{ StartIBot, StartJ, StartKBot },
						Step,
						{ SizeXBot, SizeY, SizeZBot },
						ArraySize);

					GetChild(StartX, StartY, StartZ + SizeZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartXBot, StartY, StartZTop },
					{ StartIBot, StartJ, StartKTop },
						Step,
						{ SizeXBot, SizeY, SizeZTop },
						ArraySize);


					GetChild(StartX + SizeX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartXTop, StartY, StartZBot },
					{ StartITop, StartJ, StartKBot },
						Step,
						{ SizeXTop, SizeY, SizeZBot },
						ArraySize);

					GetChild(StartX + SizeX, StartY, StartZ + SizeZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartXTop, StartY, StartZTop },
					{ StartITop, StartJ, StartKTop },
						Step,
						{ SizeXTop, SizeY, SizeZTop },
						ArraySize);
				}
			}
			else
			{
				// Split Y

				if (StartZ + SizeZ < Position.Z || Position.Z <= StartZ)
				{
					GetChild(StartX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartXBot, StartYBot, StartZ },
					{ StartIBot, StartJBot, StartK },
						Step,
						{ SizeXBot, SizeYBot, SizeZ },
						ArraySize);

					GetChild(StartX, StartY + SizeY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartXBot, StartYTop, StartZ },
					{ StartIBot, StartJTop, StartK },
						Step,
						{ SizeXBot, SizeYTop, SizeZ },
						ArraySize);

					GetChild(StartX + SizeX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartXTop, StartYBot, StartZ },
					{ StartITop, StartJBot, StartK },
						Step,
						{ SizeXTop, SizeYBot, SizeZ },
						ArraySize);

					GetChild(StartX + SizeX, StartY + SizeY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartXTop, StartYTop, StartZ },
					{ StartITop, StartJTop, StartK },
						Step,
						{ SizeXTop, SizeYTop, SizeZ },
						ArraySize);
				}
				else
				{
					// Split Z

					GetChild(StartX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartXBot, StartYBot, StartZBot },
					{ StartIBot, StartJBot, StartKBot },
						Step,
						{ SizeXBot, SizeYBot, SizeZBot },
						ArraySize);

					GetChild(StartX, StartY, StartZ + SizeZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartXBot, StartYBot, StartZTop },
					{ StartIBot, StartJBot, StartKTop },
						Step,
						{ SizeXBot, SizeYBot, SizeZTop },
						ArraySize);


					GetChild(StartX, StartY + SizeY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartXBot, StartYTop, StartZBot },
					{ StartIBot, StartJTop, StartKBot },
						Step,
						{ SizeXBot, SizeYTop, SizeZBot },
						ArraySize);

					GetChild(StartX, StartY + SizeY, StartZ + SizeZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartXBot, StartYTop, StartZTop },
					{ StartIBot, StartJTop, StartKTop },
						Step,
						{ SizeXBot, SizeYTop, SizeZTop },
						ArraySize);





					GetChild(StartX + SizeX, StartY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartXTop, StartYBot, StartZBot },
					{ StartITop, StartJBot, StartKBot },
						Step,
						{ SizeXTop, SizeYBot, SizeZBot },
						ArraySize);

					GetChild(StartX + SizeX, StartY, StartZ + SizeZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartXTop, StartYBot, StartZTop },
					{ StartITop, StartJBot, StartKTop },
						Step,
						{ SizeXTop, SizeYBot, SizeZTop },
						ArraySize);


					GetChild(StartX + SizeX, StartY + SizeY, StartZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartXTop, StartYTop, StartZBot },
					{ StartITop, StartJTop, StartKBot },
						Step,
						{ SizeXTop, SizeYTop, SizeZBot },
						ArraySize);

					GetChild(StartX + SizeX, StartY + SizeY, StartZ + SizeZ)->GetValuesAndMaterials(InValues, InMaterials,
					{ StartXTop, StartYTop, StartZTop },
					{ StartITop, StartJTop, StartKTop },
						Step,
						{ SizeXTop, SizeYTop, SizeZTop },
						ArraySize);
				}
			}
		}
	}
}

void FValueOctree::SetValueAndMaterial(int X, int Y, int Z, float Value, FVoxelMaterial Material, bool bSetValue, bool bSetMaterial)
{
	check(IsLeaf());
	check(IsInOctree(X, Y, Z));

	bIsNetworkDirty = true;

	if (Depth != 0)
	{
		CreateChilds();
		bIsDirty = true;
		GetChild(X, Y, Z)->SetValueAndMaterial(X, Y, Z, Value, Material, bSetValue, bSetMaterial);
	}
	else
	{
		if (!IsDirty())
		{
			SetAsDirty();
		}

		int LocalX, LocalY, LocalZ;
		GlobalToLocal(X, Y, Z, LocalX, LocalY, LocalZ);

		int Index = IndexFromCoordinates(LocalX, LocalY, LocalZ);
		if (bSetValue)
		{
			Values[Index] = Value;

			if (bMultiplayer)
			{
				DirtyValues.Add(Index);
			}
		}
		if (bSetMaterial)
		{
			Materials[Index] = Material;

			if (bMultiplayer)
			{
				DirtyMaterials.Add(Index);
			}
		}
	}
}

void FValueOctree::AddDirtyChunksToSaveList(std::list<TSharedRef<FVoxelChunkSave>>& SaveList)
{
	check(!IsLeaf() == (Childs.Num() == 8));
	check(!(IsDirty() && IsLeaf() && Depth != 0));

	if (IsDirty())
	{
		if (IsLeaf())
		{
			auto SaveStruct = TSharedRef<FVoxelChunkSave>(new FVoxelChunkSave(Id, Position, Values, Materials));
			SaveList.push_back(SaveStruct);
		}
		else
		{
			for (auto Child : Childs)
			{
				Child->AddDirtyChunksToSaveList(SaveList);
			}
		}
	}
}

void FValueOctree::LoadFromSaveAndGetModifiedPositions(std::list<FVoxelChunkSave>& Save, std::forward_list<FIntVector>& OutModifiedPositions)
{
	if (Save.empty())
	{
		return;
	}

	if (Depth == 0)
	{
		if (Save.front().Id == Id)
		{
			bIsDirty = true;
			Values = Save.front().Values;
			Materials = Save.front().Materials;
			Save.pop_front();

			check(Values.Num() == 4096);
			check(Materials.Num() == 4096);

			// Update neighbors
			const int S = Size();
			OutModifiedPositions.push_front(Position - FIntVector(0, 0, 0));
			OutModifiedPositions.push_front(Position - FIntVector(S, 0, 0));
			OutModifiedPositions.push_front(Position - FIntVector(0, S, 0));
			OutModifiedPositions.push_front(Position - FIntVector(S, S, 0));
			OutModifiedPositions.push_front(Position - FIntVector(0, 0, S));
			OutModifiedPositions.push_front(Position - FIntVector(S, 0, S));
			OutModifiedPositions.push_front(Position - FIntVector(0, S, S));
			OutModifiedPositions.push_front(Position - FIntVector(S, S, S));
		}
	}
	else
	{
		uint64 Pow = IntPow9(Depth);
		if (Id / Pow == Save.front().Id / Pow)
		{
			if (IsLeaf())
			{
				CreateChilds();
				bIsDirty = true;
			}
			for (auto Child : Childs)
			{
				Child->LoadFromSaveAndGetModifiedPositions(Save, OutModifiedPositions);
			}
		}
	}
}

void FValueOctree::AddChunksToDiffLists(std::forward_list<FVoxelValueDiff>& OutValueDiffList, std::forward_list<FVoxelMaterialDiff>& OutColorDiffList)
{
	if (IsLeaf())
	{
		if (bIsNetworkDirty)
		{
			bIsNetworkDirty = false;

			for (int Index : DirtyValues)
			{
				OutValueDiffList.push_front(FVoxelValueDiff(Id, Index, Values[Index]));
			}
			for (int Index : DirtyMaterials)
			{
				OutColorDiffList.push_front(FVoxelMaterialDiff(Id, Index, Materials[Index]));
			}
			DirtyValues.Empty(4096);
			DirtyMaterials.Empty(4096);
		}
	}
	else
	{
		for (auto Child : Childs)
		{
			Child->AddChunksToDiffLists(OutValueDiffList, OutColorDiffList);
		}
	}
}

void FValueOctree::LoadFromDiffListsAndGetModifiedPositions(std::forward_list<FVoxelValueDiff>& ValuesDiffs, std::forward_list<FVoxelMaterialDiff>& MaterialsDiffs, std::forward_list<FIntVector>& OutModifiedPositions)
{
	if (ValuesDiffs.empty() && MaterialsDiffs.empty())
	{
		return;
	}

	if (Depth == 0)
	{
		// Values
		while (!ValuesDiffs.empty() && ValuesDiffs.front().Id == Id)
		{
			if (!IsDirty())
			{
				SetAsDirty();
			}

			Values[ValuesDiffs.front().Index] = ValuesDiffs.front().Value;

			int X, Y, Z;
			CoordinatesFromIndex(ValuesDiffs.front().Index, X, Y, Z);
			OutModifiedPositions.push_front(FIntVector(X, Y, Z) + GetMinimalCornerPosition());

			ValuesDiffs.pop_front();
		}
		// Colors
		while (!MaterialsDiffs.empty() && MaterialsDiffs.front().Id == Id)
		{
			if (!IsDirty())
			{
				SetAsDirty();
			}

			Materials[MaterialsDiffs.front().Index] = MaterialsDiffs.front().Material;

			int X, Y, Z;
			CoordinatesFromIndex(MaterialsDiffs.front().Index, X, Y, Z);
			OutModifiedPositions.push_front(FIntVector(X, Y, Z) + GetMinimalCornerPosition());

			MaterialsDiffs.pop_front();
		}
	}
	else
	{
		uint64 Pow = IntPow9(Depth);
		if ((!ValuesDiffs.empty() && Id / Pow == ValuesDiffs.front().Id / Pow) || (!MaterialsDiffs.empty() && Id / Pow == MaterialsDiffs.front().Id / Pow))
		{
			if (IsLeaf())
			{
				bIsDirty = true;
				CreateChilds();
			}
			for (auto Child : Childs)
			{
				Child->LoadFromDiffListsAndGetModifiedPositions(ValuesDiffs, MaterialsDiffs, OutModifiedPositions);
			}
		}
	}
}


void FValueOctree::CreateChilds()
{
	check(IsLeaf());
	check(Childs.Num() == 0);
	check(Depth != 0);

	int d = Size() / 4;
	uint64 Pow = IntPow9(Depth - 1);

	Childs.Add(new FValueOctree(WorldGenerator, Position + FIntVector(-d, -d, -d), Depth - 1, Id + 1 * Pow, bMultiplayer));
	Childs.Add(new FValueOctree(WorldGenerator, Position + FIntVector(+d, -d, -d), Depth - 1, Id + 2 * Pow, bMultiplayer));
	Childs.Add(new FValueOctree(WorldGenerator, Position + FIntVector(-d, +d, -d), Depth - 1, Id + 3 * Pow, bMultiplayer));
	Childs.Add(new FValueOctree(WorldGenerator, Position + FIntVector(+d, +d, -d), Depth - 1, Id + 4 * Pow, bMultiplayer));
	Childs.Add(new FValueOctree(WorldGenerator, Position + FIntVector(-d, -d, +d), Depth - 1, Id + 5 * Pow, bMultiplayer));
	Childs.Add(new FValueOctree(WorldGenerator, Position + FIntVector(+d, -d, +d), Depth - 1, Id + 6 * Pow, bMultiplayer));
	Childs.Add(new FValueOctree(WorldGenerator, Position + FIntVector(-d, +d, +d), Depth - 1, Id + 7 * Pow, bMultiplayer));
	Childs.Add(new FValueOctree(WorldGenerator, Position + FIntVector(+d, +d, +d), Depth - 1, Id + 8 * Pow, bMultiplayer));

	bHasChilds = true;
	check(!IsLeaf() == (Childs.Num() == 8));
}

void FValueOctree::SetAsDirty()
{
	check(!IsDirty());
	check(Depth == 0);

	Values.SetNumUninitialized(16 * 16 * 16);
	Materials.SetNumUninitialized(16 * 16 * 16);
	for (int X = 0; X < 16; X++)
	{
		for (int Y = 0; Y < 16; Y++)
		{
			for (int Z = 0; Z < 16; Z++)
			{
				int GlobalX, GlobalY, GlobalZ;
				LocalToGlobal(X, Y, Z, GlobalX, GlobalY, GlobalZ);

				float Value = 0;
				FVoxelMaterial Material;
				//GetValueAndMaterial(GlobalX, GlobalY, GlobalZ, Value, Material); // TODO
				Values[X + 16 * Y + 16 * 16 * Z] = Value;
				Materials[X + 16 * Y + 16 * 16 * Z] = Material;
			}
		}
	}
	bIsDirty = true;
}

FORCEINLINE int FValueOctree::IndexFromCoordinates(int X, int Y, int Z) const
{
	return X + 16 * Y + 16 * 16 * Z;
}

FORCEINLINE void FValueOctree::CoordinatesFromIndex(int Index, int& OutX, int& OutY, int& OutZ) const
{
	OutX = Index % 16;

	Index = (Index - OutX) / 16;
	OutY = Index % 16;

	Index = (Index - OutY) / 16;
	OutZ = Index;
}

FValueOctree * FValueOctree::GetChild(int X, int Y, int Z) const
{
	check(!IsLeaf());
	// Ex: Child 6 -> position (0, 1, 1) -> 0b011 == 6
	return Childs[(X >= Position.X) + 2 * (Y >= Position.Y) + 4 * (Z >= Position.Z)];
}

FValueOctree* FValueOctree::GetLeaf(int X, int Y, int Z)
{
	check(IsInOctree(X, Y, Z));

	FValueOctree* Ptr = this;

	while (!Ptr->IsLeaf())
	{
		Ptr = Ptr->GetChild(X, Y, Z);
	}

	check(Ptr->IsInOctree(X, Y, Z));

	return Ptr;
}

void FValueOctree::GetDirtyChunksPositions(std::forward_list<FIntVector>& OutPositions)
{
	if (IsDirty())
	{
		if (IsLeaf())
		{
			// With neighbors
			const int S = Size();
			OutPositions.push_front(Position - FIntVector(0, 0, 0));
			OutPositions.push_front(Position - FIntVector(S, 0, 0));
			OutPositions.push_front(Position - FIntVector(0, S, 0));
			OutPositions.push_front(Position - FIntVector(S, S, 0));
			OutPositions.push_front(Position - FIntVector(0, 0, S));
			OutPositions.push_front(Position - FIntVector(S, 0, S));
			OutPositions.push_front(Position - FIntVector(0, S, S));
			OutPositions.push_front(Position - FIntVector(S, S, S));
		}
		else
		{
			for (auto Child : Childs)
			{
				Child->GetDirtyChunksPositions(OutPositions);
			}
		}
	}
}

//ValueOctree* ValueOctree::GetCopy()
//{
//	ValueOctree*  NewOctree = new ValueOctree(WorldGenerator, Position, Depth, Id);
//
//	if (Depth == 0)
//	{
//		TArray<float, TFixedAllocator<16 * 16 * 16>> ValuesCopy;
//		TArray<FColor, TFixedAllocator<16 * 16 * 16>> ColorsCopy;
//
//		if (IsDirty())
//		{
//			ValuesCopy.SetNumUninitialized(4096);
//			ColorsCopy.SetNumUninitialized(4096);
//
//			for (int X = 0; X < 16; X++)
//			{
//				for (int Y = 0; Y < 16; Y++)
//				{
//					for (int Z = 0; Z < 16; Z++)
//					{
//						ValuesCopy[X + 16 * Y + 16 * 16 * Z] = Values[X + 16 * Y + 16 * 16 * Z];
//						ColorsCopy[X + 16 * Y + 16 * 16 * Z] = Colors[X + 16 * Y + 16 * 16 * Z];
//					}
//				}
//			}
//		}
//
//		NewOctree->Values = ValuesCopy;
//		NewOctree->Colors = ColorsCopy;
//		NewOctree->bHasChilds = false;
//		NewOctree->bIsDirty = bIsDirty;
//	}
//	else if (IsLeaf())
//	{
//		NewOctree->bHasChilds = false;
//		check(!bIsDirty);
//		NewOctree->bIsDirty = false;
//	}
//	else
//	{
//		NewOctree->bHasChilds = true;
//		check(bIsDirty);
//		NewOctree->bIsDirty = true;
//		for (auto Child : Childs)
//		{
//			NewOctree->Childs.Add(Child->GetCopy());
//		}
//	}
//	return NewOctree;
//}
