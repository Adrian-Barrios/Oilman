#include "worldGenerator.h"
#include <FastNoiseSIMD.h>
#include <memory>

// The texture atlas is fully used by the existing block list, so there is no
// dedicated oil/gas art yet. These reuse existing tiles purely so the deposits
// are visible; swap them once the real textures exist.
static const std::uint16_t oilDepositBlock = Block::clay;
static const std::uint16_t gasDepositBlock = Block::ice;

void generateWorld(GameMap& gameMap, int seed)
{
	const int w = 900;
	const int h = 500;

	gameMap.create(w, h);

#pragma region noiseConfig
	std::unique_ptr<FastNoiseSIMD> dirtNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());
	std::unique_ptr<FastNoiseSIMD> stoneNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());
	std::unique_ptr<FastNoiseSIMD> depositNoiseGenerator(FastNoiseSIMD::NewFastNoiseSIMD());

	dirtNoiseGenerator->SetSeed(seed++);
	stoneNoiseGenerator->SetSeed(seed++);
	depositNoiseGenerator->SetSeed(seed++);

	dirtNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::Simplex);
	dirtNoiseGenerator->SetFrequency(0.015);

	// Few octaves and a low frequency give broad rolling hills, where the 4
	// octave fractal was producing jagged ridges.
	stoneNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
	stoneNoiseGenerator->SetFractalOctaves(2);
	stoneNoiseGenerator->SetFractalGain(0.35);
	stoneNoiseGenerator->SetFrequency(0.005);

	// Cellular noise gives discrete blobs rather than a continuous field, so
	// each cell can become one self contained pocket. Scaling the y axis up
	// raises the frequency along y only, squashing those cells into the wide
	// flat lenses a deposit should look like.
	depositNoiseGenerator->SetNoiseType(FastNoiseSIMD::NoiseType::Cellular);
	depositNoiseGenerator->SetCellularReturnType(FastNoiseSIMD::CellularReturnType::CellValue);
	depositNoiseGenerator->SetCellularDistanceFunction(FastNoiseSIMD::CellularDistanceFunction::Natural);
	depositNoiseGenerator->SetCellularJitter(0.45);
	depositNoiseGenerator->SetFrequency(0.03);
	depositNoiseGenerator->SetAxisScales(1.0, 3.5, 1.0);

	// Warping the sample position breaks up the straight Voronoi cell edges,
	// so pockets end up with organic outlines instead of flat polygon sides.
	depositNoiseGenerator->SetPerturbType(FastNoiseSIMD::PerturbType::Gradient);
	depositNoiseGenerator->SetPerturbAmp(50);
	depositNoiseGenerator->SetPerturbFrequency(0.06);

	float* dirtNoise = FastNoiseSIMD::GetEmptySet(w);
	float* stoneNoise = FastNoiseSIMD::GetEmptySet(w);
	float* depositNoise = FastNoiseSIMD::GetEmptySet(w, h, 1);

	dirtNoiseGenerator->FillNoiseSet(dirtNoise, 0, 0, 0, w, 1, 1);
	stoneNoiseGenerator->FillNoiseSet(stoneNoise, 0, 0, 0, w, 1, 1);
	depositNoiseGenerator->FillNoiseSet(depositNoise, 0, 0, 0, w, h, 1);

	for (int i = 0; i < w; i++)
	{
		dirtNoise[i] = (dirtNoise[i] + 1) / 2;
		stoneNoise[i] = (stoneNoise[i] + 1) / 2;

		// Cubing the signed noise pulls mid range values back toward the
		// centre, so most of the world sits near level and only the strongest
		// peaks and dips still stand out.
		float s = stoneNoise[i] * 2 - 1;
		s = s * s * s;
		stoneNoise[i] = (s + 1) / 2;
	}

	int dirtOffsetStart = 6;
	int dirtOffsetEnd = 20;

	// A 30 block spread instead of the previous 90 keeps the surface gentle.
	int stoneHeightStart = 110;
	int stoneHeightEnd = 140;

	// Every cell holds one constant value, so thresholding picks whole pockets
	// rather than cutting them in half.
	const float oilThreshold = 0.86;
	const float gasThreshold = 0.94;

	// Keeps pockets clear of the stone/dirt boundary so they read as buried.
	const int depositDepthMargin = 12;
#pragma endregion

	for (int x = 0; x < w; x++)
	{
		int stoneHeight = stoneHeightStart + (stoneHeightEnd - stoneHeightStart) * stoneNoise[x];
		int dirtHeight = dirtOffsetStart + (dirtOffsetEnd - dirtOffsetStart) * dirtNoise[x];
		dirtHeight = stoneHeight - dirtHeight;

		for (int y = 0; y < h; y++)
		{
			Block b;

			if (y > dirtHeight)
			{
				b.type = Block::dirt;
			}

			if (y == dirtHeight)
			{
				b.type = Block::grassBlock;
			}

			if (y > stoneHeight)
			{
				b.type = Block::stone;
			}

			// Deposits replace stone only, never dirt or air.
			if (b.type == Block::stone && y > stoneHeight + depositDepthMargin)
			{
				float deposit = (depositNoise[x * h + y] + 1) / 2;

				if (deposit > gasThreshold)
				{
					b.type = gasDepositBlock;
				}
				else if (deposit > oilThreshold)
				{
					b.type = oilDepositBlock;
				}
			}

			gameMap.getBlocUnsafe(x, y) = b;
		}
	}

	FastNoiseSIMD::FreeNoiseSet(dirtNoise);
	FastNoiseSIMD::FreeNoiseSet(stoneNoise);
	FastNoiseSIMD::FreeNoiseSet(depositNoise);
}
