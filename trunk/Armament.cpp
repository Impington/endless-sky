/* Armament.cpp
Michael Zahniser, 11 Feb 2014

Function definitions for the Armament class.
*/

#include "Armament.h"

#include "Effect.h"
#include "Outfit.h"
#include "Projectile.h"
#include "Ship.h"

#include <cmath>
#include <limits>

using namespace std;



// Members of Armament::Weapon:

// Constructor.
Armament::Weapon::Weapon(const Point &point, bool isTurret)
	: outfit(nullptr), point(point * .5), reload(0), isTurret(isTurret)
{
}



// Check if anything is installed in this gun port.
bool Armament::Weapon::HasOutfit() const
{
	return (outfit != nullptr);
}



// Don't call this without checking HasOutfit()!
const Outfit *Armament::Weapon::GetOutfit() const
{
	return outfit;
}



// Get the point, in ship image coordinates, from which projectiles of
// this weapon should originate.
const Point &Armament::Weapon::GetPoint() const
{
	return point;
}



// Shortcuts for querying weapon characteristics.
bool Armament::Weapon::IsTurret() const
{
	return isTurret;
}



bool Armament::Weapon::IsHoming() const
{
	return outfit && outfit->WeaponGet("homing");
}



bool Armament::Weapon::IsAntiMissile() const
{
	return outfit && outfit->WeaponGet("anti-missile") >= 1.;
}



// Check if this weapon is ready to fire.
bool Armament::Weapon::IsReady() const
{
	return outfit && reload <= 0;
}



// Perform one step (i.e. decrement the reload count).
void Armament::Weapon::Step()
{
	if(reload)
		--reload;
}



// Fire this weapon. If it is a turret, it automatically points toward
// the given ship's target. If the weapon requires ammunition, it will
// be subtracted from the given ship.
void Armament::Weapon::Fire(Ship &ship, list<Projectile> &projectiles)
{
	// Since this is only called internally by Armament (no one else has non-
	// const access), assume Armament checked that this is a valid call.
	Angle aim = ship.Facing();
	
	Point start = ship.Position() + aim.Rotate(point);
	
	shared_ptr<const Ship> target = ship.GetTargetShip().lock();
	
	if(!isTurret || !target)
		aim += angle;
	else
	{
		Point p = target->Position() - start;
		Point v = target->Velocity() - ship.Velocity();
		double steps = RendevousTime(p, v, outfit->WeaponGet("velocity"));
		
		// Special case: RendevousTime() may return NaN. But in that case, this
		// comparison will return false. Also, if the target is out of range,
		// just fire toward its current location instead of extrapolating way
		// into the future.
		if(steps < outfit->WeaponGet("lifetime"))
			p += steps * v;
		
		aim = Angle((180. / M_PI) * atan2(p.X(), -p.Y()));
	}
	
	projectiles.emplace_back(ship, start, aim, outfit);
	double force = outfit->WeaponGet("firing force");
	if(force)
		ship.ApplyForce(aim.Unit() * -force);
	
	// Reset the reload count.
	reload += outfit->WeaponGet("reload");
	ship.ExpendAmmo(outfit);
}



// Fire an anti-missile. Returns true if the missile should be killed.
bool Armament::Weapon::FireAntiMissile(Ship &ship, const Projectile &projectile, list<Effect> &effects)
{
	int strength = outfit->WeaponGet("anti-missile");
	if(!strength)
		return false;
	
	double range = outfit->WeaponGet("velocity");
	
	// Check if the missile is in range.
	Point start = ship.Position() + ship.Facing().Rotate(point);
	Point offset = projectile.Position() - start;
	if(offset.Length() > range)
		return false;
	
	// Figure out where the effect should be placed. Anti-missiles do not create
	// projectiles; they just create a blast animation.
	start += (.5 * range) * offset.Unit();
	Angle aim = (180. / M_PI) * atan2(offset.X(), -offset.Y());
	for(const auto &eit : outfit->HitEffects())
		for(int i = 0; i < eit.second; ++i)
		{
			effects.push_back(*eit.first);
			effects.back().Place(start, ship.Velocity(), aim);
		}
	
	// Reset the reload count.
	reload += outfit->WeaponGet("reload");
	ship.ExpendAmmo(outfit);
	
	return (rand() % strength > rand() % projectile.MissileStrength());
}



// Install a weapon here (assuming it is empty). This is only for
// Armament to call internally.
void Armament::Weapon::Install(const Outfit *outfit)
{
	if(outfit && outfit->IsWeapon() && (!outfit->Get("turret mounts") || isTurret))
	{
		this->outfit = outfit;
		
		if(!isTurret)
		{
			// Find the point of convergence of shots fired from
			// this gun. The shots should converge at distance d:
			double d = outfit->WeaponGet("range") * .9;
			// The angle is therefore:
			angle = Angle(asin(point.X() * .5 / d) * (180. / M_PI));
		}
	}
}



// Uninstall the outfit from this port (if it has one).
void Armament::Weapon::Uninstall()
{
	outfit = nullptr;
}



// Members of Armament:

// Add a gun or turret hard-point.
void Armament::AddGunPort(const Point &point)
{
	weapons.emplace_back(point, false);
}



void Armament::AddTurret(const Point &point)
{
	weapons.emplace_back(point, true);
}



// This must be called after all the outfit data is loaded. If you add more
// of a given weapon than there are slots for it, the extras will not fire.
// But, the "gun ports" attribute should keep that from happening.
void Armament::Add(const Outfit *outfit, int count)
{
	if(!count || !outfit || !outfit->IsWeapon())
		return;
	
	int installed = 0;
	bool isTurret = outfit->Get("turret mounts");
	
	if(count < 0)
	{
		// Look for slots where this weapon is installed.
		for(Weapon &weapon : weapons)
			if(weapon.GetOutfit() == outfit)
			{
				weapon.Uninstall();
				if(--installed == count)
					break;
			}
	}
	else
	{
		// Look for empty, compatible slots.
		for(Weapon &weapon : weapons)
			if(!weapon.GetOutfit() && weapon.IsTurret() == isTurret)
			{
				weapon.Install(outfit);
				if(++installed == count)
					break;
			}
	}
	
	// If this weapon is streamed, create a stream counter. Missiles and anti-
	// missiles do not stream.
	if(!outfit->WeaponGet("missile strength") && !outfit->WeaponGet("anti-missile"))
	{
		auto it = streamReload.find(outfit);
		if(it == streamReload.end())
			streamReload[outfit] = count;
		else
		{
			it->second += count;
			if(!it->second)
				streamReload.erase(it);
		}
	}
}



// Access the array of weapons.
const vector<Armament::Weapon> &Armament::Get() const
{
	return weapons;
}



// Fire the given weapon, if it is ready. If it did not fire because it is
// not ready, return false.
void Armament::Fire(int index, Ship &ship, list<Projectile> &projectiles)
{
	if(index < 0 || index >= weapons.size() || !weapons[index].IsReady())
		return;
	
	auto it = streamReload.find(weapons[index].GetOutfit());
	if(it != streamReload.end() && it->second > 0)
		return;
	
	weapons[index].Fire(ship, projectiles);
	if(it != streamReload.end())
		it->second += weapons[index].GetOutfit()->WeaponGet("reload");
}



bool Armament::FireAntiMissile(int index, Ship &ship, const Projectile &projectile, std::list<Effect> &effects)
{
	if(index < 0 || index >= weapons.size() || !weapons[index].IsReady())
		return false;
	
	return weapons[index].FireAntiMissile(ship, projectile, effects);
}



// Update the reload counters.
void Armament::Step(const Ship &ship)
{
	for(Weapon &weapon : weapons)
		weapon.Step();
	
	for(auto &it : streamReload)
		if(it.second > 0)
			it.second -= ship.OutfitCount(it.first);
}



// Get the amount of time it would take the given weapon to reach the given
// target, assuming it can be fired in any direction (i.e. turreted). For
// non-turreted weapons this can be used to calculate the ideal direction to
// point the ship in.
double Armament::RendevousTime(const Point &p, const Point &v, double vp)
{
	// How many steps will it take this projectile
	// to intersect the target?
	// (p.x + v.x*t)^2 + (p.y + v.y*t)^2 = vp^2*t^2
	// p.x^2 + 2*p.x*v.x*t + v.x^2*t^2
	//    + p.y^2 + 2*p.y*v.y*t + v.y^2t^2
	//    - vp^2*t^2 = 0
	// (v.x^2 + v.y^2 - vp^2) * t^2
	//    + (2 * (p.x * v.x + p.y * v.y)) * t
	//    + (p.x^2 + p.y^2) = 0
	double a = v.Dot(v) - vp * vp;
	double b = 2. * p.Dot(v);
	double c = p.Dot(p);
	double discriminant = b * b - 4 * a * c;
	if(discriminant < 0.)
		return numeric_limits<double>::quiet_NaN();

	discriminant = sqrt(discriminant);

	// The solutions are b +- discriminant.
	// But it's not a solution if it's negative.
	double r1 = (-b + discriminant) / (2. * a);
	double r2 = (-b - discriminant) / (2. * a);
	if(r1 >= 0. && r2 >= 0.)
		return min(r1, r2);
	else if(r1 >= 0. || r2 >= 0.)
		return max(r1, r2);

	return numeric_limits<double>::quiet_NaN();
}
