/**
 * Centralized Domain and URL Configuration for Baddie
 *
 * Configurable via environment variables with fallbacks to the currently owned domain.
 */

export const ACTIVE_OWNED_DOMAIN = "baddieapp.co.uk";

export const KNOWN_BRAND_DOMAINS = [
  "baddieapp.co.uk",
  "baddieapp.com",
  "baddieapp.ng",
  "management.baddieapp.co.uk"
] as const;

export type KnownBrandDomain = (typeof KNOWN_BRAND_DOMAINS)[number];

export type EnvRecord = Record<string, string | undefined>;

function getEnv(env?: EnvRecord): EnvRecord {
  if (env) return env;
  try {
    const proc = (globalThis as { process?: { env?: EnvRecord } }).process;
    if (proc?.env) {
      return proc.env;
    }
  } catch {
    return {};
  }
  return {};
}

function getEnvValue(env: EnvRecord, ...keys: string[]): string | null {
  try {
    for (const key of keys) {
      const val = env[key]?.trim();
      if (val) return val;
    }
  } catch {
    return null;
  }
  return null;
}

function stripTrailingSlash(url: string): string {
  return url.replace(/\/+$/, "");
}

/**
 * Normalizes hostname: removes port, whitespace, and lowercases.
 */
export function normalizeDomainHostname(hostname: string | null | undefined): string | null {
  if (!hostname) return null;
  const normalized = hostname.trim().toLowerCase();
  if (!normalized) return null;
  return normalized.split(":")[0] ?? null;
}

/**
 * Checks if a hostname belongs to any known Baddie brand domain or subdomain.
 */
export function isTrustedBaddieDomain(
  hostname: string | null | undefined,
  extraDomains: string[] = []
): boolean {
  const norm = normalizeDomainHostname(hostname);
  if (!norm) return false;
  // An old configuration must not re-authorize a domain we do not own.
  if (norm === "baddie.ng" || norm.endsWith(".baddie.ng")) return false;

  const allDomains = [...KNOWN_BRAND_DOMAINS, ...extraDomains.map((d) => d.toLowerCase().trim()).filter(Boolean)];

  return allDomains.some((domain) => norm === domain || norm.endsWith(`.${domain}`));
}

/**
 * Resolves the primary public web URL.
 */
export function resolveSiteUrl(env?: EnvRecord): string {
  const resolvedEnv = getEnv(env);
  const explicit = getEnvValue(
    resolvedEnv,
    "EXPO_PUBLIC_SITE_URL",
    "NEXT_PUBLIC_SITE_URL",
    "SITE_URL",
    "APP_BASE_URL",
    "VITE_SITE_URL"
  );
  if (explicit) {
    const stripped = stripTrailingSlash(explicit);
    try {
      const parsed = new URL(stripped);
      const host = parsed.hostname.toLowerCase();
      if (host === "baddie.ng" || host.endsWith(".baddie.ng")) {
        return `https://${ACTIVE_OWNED_DOMAIN}`;
      }
    } catch {
      // ignore invalid URLs and return stripped
    }
    return stripped;
  }
  return `https://${ACTIVE_OWNED_DOMAIN}`;
}

/**
 * Normalizes a share URL to ensure it always uses the primary production domain (baddieapp.co.uk)
 * and never the legacy repository name (baddie.ng) or non-live domain (baddieapp.ng).
 */
export function normalizeShareUrl(url: string | null | undefined): string {
  if (!url) return `https://${ACTIVE_OWNED_DOMAIN}`;
  try {
    const parsed = new URL(url);
    if (parsed.protocol !== "http:" && parsed.protocol !== "https:") {
      return `https://${ACTIVE_OWNED_DOMAIN}`;
    }
    const host = parsed.hostname.toLowerCase();
    if (host === "app.baddieapp.co.uk" || host === "shop.baddieapp.co.uk" || host === "baddie.ng" || host.endsWith(".baddie.ng") || host === "baddieapp.ng" || host.endsWith(".baddieapp.ng")) {
      parsed.hostname = ACTIVE_OWNED_DOMAIN;
    }
    return stripTrailingSlash(parsed.toString());
  } catch {
    return `https://${ACTIVE_OWNED_DOMAIN}`;
  }
}


/**
 * Resolves the mobile web app URL.
 */
export function resolveMobileWebUrl(env?: EnvRecord): string {
  const resolvedEnv = getEnv(env);
  const explicit = getEnvValue(
    resolvedEnv,
    "EXPO_PUBLIC_MOBILE_WEB_URL",
    "NEXT_PUBLIC_DEFAULT_MOBILE_WEB_URL",
    "NEXT_PUBLIC_MOBILE_WEB_PREVIEW_URL",
    "VITE_DEFAULT_MOBILE_WEB_URL",
    "MOBILE_WEB_URL"
  );
  if (explicit) {
    return normalizeShareUrl(explicit);
  }
  return `https://${ACTIVE_OWNED_DOMAIN}`;
}

/**
 * Resolves the admin portal URL.
 */
export function resolveAdminSiteUrl(env?: EnvRecord): string {
  const resolvedEnv = getEnv(env);
  const explicit = getEnvValue(
    resolvedEnv,
    "NEXT_PUBLIC_ADMIN_URL",
    "NEXT_PUBLIC_SITE_URL",
    "EXPO_PUBLIC_ADMIN_URL",
    "ADMIN_SITE_URL",
    "ADMIN_BASE_URL"
  );
  if (explicit) {
    return stripTrailingSlash(explicit);
  }
  return `https://management.${ACTIVE_OWNED_DOMAIN}`;
}

/**
 * Resolves the Pay-For-Me checkout base URL.
 */
export function resolvePayForMeUrl(env?: EnvRecord): string {
  const resolvedEnv = getEnv(env);
  const explicit = getEnvValue(resolvedEnv, "EXPO_PUBLIC_PAY_FOR_ME_BASE_URL", "PAY_FOR_ME_BASE_URL");
  if (explicit) {
    return stripTrailingSlash(explicit);
  }
  return `${resolveAdminSiteUrl(env)}/pay-for-me`;
}

/**
 * Generates product share URL.
 */
export function resolveProductShareUrl(productId: string, env?: EnvRecord): string {
  return `${resolveSiteUrl(env)}/p/${encodeURIComponent(productId)}`;
}

/**
 * Generates creator/user profile share URL.
 */
export function resolveProfileShareUrl(handle: string, env?: EnvRecord): string {
  const cleanHandle = handle.startsWith("@") ? handle.slice(1) : handle;
  return `${resolveSiteUrl(env)}/u/${encodeURIComponent(cleanHandle)}`;
}

/**
 * Generates post/media share URL.
 */
export function resolvePostShareUrl(postId: string, env?: EnvRecord): string {
  return `${resolveSiteUrl(env)}/posts/${encodeURIComponent(postId)}`;
}

/**
 * Generates pack share URL.
 */
export function resolvePackShareUrl(packId: string, env?: EnvRecord): string {
  return `${resolveSiteUrl(env)}/packs/${encodeURIComponent(packId)}`;
}

/**
 * Generates public order tracking URL.
 */
export function resolveOrderTrackingUrl(orderNumber: string, env?: EnvRecord): string {
  return `${resolveMobileWebUrl(env)}/app/you?overlay=orderTracking&orderId=${encodeURIComponent(orderNumber)}`;
}

/**
 * Generates order receipt URL.
 */
export function resolveReceiptUrl(orderId: string, env?: EnvRecord): string {
  return `${resolveSiteUrl(env)}/receipts/${encodeURIComponent(orderId)}`;
}

/**
 * Resolves customer support email address.
 */
export function resolveSupportEmail(env?: EnvRecord): string {
  const resolvedEnv = getEnv(env);
  return (
    getEnvValue(
      resolvedEnv,
      "SUPPORT_EMAIL",
      "NEXT_PUBLIC_SUPPORT_EMAIL",
      "EXPO_PUBLIC_SUPPORT_EMAIL"
    ) ?? `support@${ACTIVE_OWNED_DOMAIN}`
  );
}

/**
 * Resolves store operations email address.
 */
export function resolveOpsEmail(env?: EnvRecord): string {
  const resolvedEnv = getEnv(env);
  return (
    getEnvValue(
      resolvedEnv,
      "OPS_EMAIL",
      "NEXT_PUBLIC_OPS_EMAIL",
      "EXPO_PUBLIC_OPS_EMAIL"
    ) ?? `ops@${ACTIVE_OWNED_DOMAIN}`
  );
}
