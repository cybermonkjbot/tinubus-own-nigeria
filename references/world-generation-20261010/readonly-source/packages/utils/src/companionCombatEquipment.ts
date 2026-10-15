/** Street equipment is account-owned. Arena loadouts never become inventory. */
export const companionStreetWeapons = [
  { id: "bat", label: "Bat", kind: "melee", damage: 27, range: 2, cooldownMs: 650, magazine: 0, reloadMs: 0, aimRadius: .5, damageFalloff: 0, reserveAmmo: 0, price: 120, stock: 24, ammoPack: 0, ammoPrice: 0, serviceKey: "street-weapon:bat" },
  { id: "pistol", label: "Pistol", kind: "firearm", damage: 25, range: 40, cooldownMs: 320, magazine: 12, reloadMs: 1700, aimRadius: .5, damageFalloff: 0, reserveAmmo: 24, price: 600, stock: 12, ammoPack: 24, ammoPrice: 60, serviceKey: "street-weapon:pistol" },
  { id: "revolver", label: "Revolver", kind: "firearm", damage: 40, range: 45, cooldownMs: 650, magazine: 6, reloadMs: 2100, aimRadius: .5, damageFalloff: 0, reserveAmmo: 18, price: 750, stock: 10, ammoPack: 18, ammoPrice: 60, serviceKey: "street-weapon:revolver" },
  { id: "smg", label: "SMG", kind: "firearm", damage: 18, range: 28, cooldownMs: 120, magazine: 30, reloadMs: 2000, aimRadius: .5, damageFalloff: 0, reserveAmmo: 60, price: 1100, stock: 8, ammoPack: 60, ammoPrice: 120, serviceKey: "street-weapon:smg" },
  { id: "shotgun", label: "Shotgun", kind: "firearm", damage: 65, range: 12, cooldownMs: 850, magazine: 5, reloadMs: 2600, aimRadius: 1.25, damageFalloff: .65, reserveAmmo: 15, price: 900, stock: 8, ammoPack: 15, ammoPrice: 75, serviceKey: "street-weapon:shotgun" },
  { id: "rifle", label: "Rifle", kind: "firearm", damage: 30, range: 65, cooldownMs: 260, magazine: 24, reloadMs: 2300, aimRadius: .5, damageFalloff: 0, reserveAmmo: 48, price: 1500, stock: 6, ammoPack: 48, ammoPrice: 120, serviceKey: "street-weapon:rifle" },
] as const;
export type CompanionStreetWeaponId = typeof companionStreetWeapons[number]["id"];
export type CompanionWeaponCarry = "stored" | "holstered" | "drawn";
export type CompanionOwnedWeapon = { weaponId: CompanionStreetWeaponId; carry: CompanionWeaponCarry; ammo: number; reserveAmmo: number };
export type CompanionEquipment = { revision: number; weapons: CompanionOwnedWeapon[] };
export type CompanionEquipmentScope = { instanceId: string; lifeId: string; revision: number };
export type CompanionEquipmentSnapshot = { equipment: CompanionEquipment; ready: boolean; busy: boolean; error: string | null };
export const emptyCompanionEquipment: CompanionEquipment = { revision: 0, weapons: [] };
export const companionStreetWeapon = (id: unknown) => companionStreetWeapons.find(weapon => weapon.id === id);
export const COMPANION_WEAPON_MAX_RESERVE_AMMO = 240;
export const carriedCompanionWeapon = (equipment: CompanionEquipment) => equipment.weapons.find(weapon => weapon.carry !== "stored") ?? null;
export const validCompanionAmmo = (ammo: number, reserveAmmo: number, weaponId: CompanionStreetWeaponId) => {
  const weapon = companionStreetWeapon(weaponId);
  return !!weapon && Number.isSafeInteger(ammo) && ammo >= 0 && ammo <= weapon.magazine
    && Number.isSafeInteger(reserveAmmo) && reserveAmmo >= 0 && reserveAmmo <= COMPANION_WEAPON_MAX_RESERVE_AMMO
    && (weapon.kind === "firearm" || ammo + reserveAmmo === 0);
};
/** A command sets a state, never toggles it. One street weapon may be carried. */
export function setCompanionWeaponCarry(equipment: CompanionEquipment, weaponId: CompanionStreetWeaponId, carry: CompanionWeaponCarry, atOwnHome: boolean): CompanionEquipment {
  const item = equipment.weapons.find(weapon => weapon.weaponId === weaponId);
  if (!item) throw new Error("This weapon is not owned.");
  if (item.carry === carry) return equipment;
  if ((carry === "stored" || item.carry === "stored") && !atOwnHome) throw new Error("Store or collect weapons at Home.");
  if (carry !== "stored" && equipment.weapons.some(weapon => weapon.weaponId !== weaponId && weapon.carry !== "stored")) throw new Error("Store the carried weapon first.");
  return { revision: equipment.revision + 1, weapons: equipment.weapons.map(weapon => weapon.weaponId === weaponId ? { ...weapon, carry } : weapon) };
}
export interface CompanionEquipmentStore {
  getSnapshot(): CompanionEquipmentSnapshot;
  subscribe(listener: () => void): () => void;
  setCarry(weaponId: CompanionStreetWeaponId, carry: CompanionWeaponCarry, scope: CompanionEquipmentScope): Promise<void>;
  dispose(): void;
}
