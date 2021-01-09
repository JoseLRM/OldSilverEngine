#include "spacestate.h"

void ShipGenerator::generateShips(SpaceState& state, f32 dt)
{
	if (gen.rate != 0u) {

		if (gen.ships.empty()) {
			gen.rate = 0u;
			timer = 0.f;
		}

		else {

			timer += dt;

			if (timer >= genRate) {
				timer -= genRate;

				u32 shipIndex = state.random.gen_u32(gen.ships.size());

				ShipType shipType = gen.ships[shipIndex].type;

				--gen.ships[shipIndex].min;
				if (gen.ships[shipIndex].min == 0u) {
					gen.ships.erase(gen.ships.begin() + shipIndex);
				}

				v2_f32 pos = state.genPos().first;
				state.createShip(pos, shipType);
			}
		}
	}
	else {
		f32 rate = shipRate;

		timer += dt;

		if (timer >= rate) {
			timer -= rate;

			u32 type = state.random.gen_u32(maxGenValue);

			ShipGeneration* newGen = nullptr;

			for (ShipGeneration& g : generations) {

				if (g.rate >= type) {
					newGen = &g;
					break;
				}
			}

			if (newGen != nullptr) {
				gen = *newGen;

				u32 shipCount = 0u;
				for (auto& s : gen.ships) {
					
					u32 count = state.random.gen_u32(s.min, s.max + 1u);
					s.min = count;
					s.max = 0u;
					shipCount += count;
				}

				genRate = gen.time / f32(shipCount);
				timer = 0u;
			}
		}
	}
}

void ShipGenerator::addDefaultGenerations()
{
	ShipGeneration g;
	g.rate = 100u;
	g.time = 1.f;
	g.ships.push_back({ 1u, 2u, ShipType_Kamikaze });
	generations.emplace_back(std::move(g));

	g.rate = 10u;
	g.time = 5.f;
	g.ships.push_back({ 5u, 20u, ShipType_Kamikaze });
	generations.emplace_back(std::move(g));

	g.rate = 10u;
	g.time = 8.f;
	g.ships.push_back({ 5u, 20u, ShipType_Shooter });
	generations.emplace_back(std::move(g));

	g.rate = 100u;
	g.time = 15.f;
	g.ships.push_back({ 5u, 10u, ShipType_Shooter });
	g.ships.push_back({ 5u, 10u, ShipType_Kamikaze });
	g.ships.push_back({ 1u, 2u, ShipType_Daddy });
	generations.emplace_back(std::move(g));

	g.rate = 30u;
	g.time = 1.f;
	g.ships.push_back({ 1u, 2u, ShipType_Shooter });
	g.ships.push_back({ 1u, 2u, ShipType_Kamikaze });
	generations.emplace_back(std::move(g));

	g.rate = 30u;
	g.time = 20.f;
	g.ships.push_back({ 20u, 50u, ShipType_Shooter });
	generations.emplace_back(std::move(g));
	
	updateGenerations();
}

void ShipGenerator::updateGenerations()
{
	if (generations.empty()) return;

	std::sort(generations.begin(), generations.end(), 
		[](ShipGeneration& g0, ShipGeneration& g1)
	{
		return g0.rate < g1.rate;
	});

	u32 count = generations.front().rate;

	for (u32 i = 1u; i < generations.size(); ++i) {

		ShipGeneration& g = generations[i];
		u32 lastCount = count;
		count += g.rate;
		g.rate += lastCount;
	}

	maxGenValue = generations.back().rate;
}