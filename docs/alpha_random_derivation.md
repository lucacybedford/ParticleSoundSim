# Derivation of $\alpha_{\text{random}}(\xi)$

The random-incidence absorption of a real, locally-reacting surface of normalised
impedance $\xi$, as a closed-form function of $\xi$. This is the function inverted
during impedance calibration (see `angle_absorption_and_scattering.md`, Part A.2).

---

## Setup

Real normalised surface impedance $\xi$, locally reacting. The pressure reflection
coefficient at incidence angle $\theta$ is

$$R(\theta) = \frac{\xi\cos\theta - 1}{\xi\cos\theta + 1}$$

and the absorption at that angle is

$$\alpha(\theta) = 1 - |R(\theta)|^2.$$

---

## Step 1 — Simplify $\alpha(\theta)$

Let $a = \xi\cos\theta$:

$$\alpha = 1 - \frac{(a-1)^2}{(a+1)^2}
        = \frac{(a+1)^2 - (a-1)^2}{(a+1)^2}
        = \frac{4a}{(a+1)^2}$$

so

$$\alpha(\theta) = \frac{4\,\xi\cos\theta}{\left(\xi\cos\theta + 1\right)^2}.$$

---

## Step 2 — Paris' formula (average over a diffuse field)

$$\alpha_{\text{random}}
  = \int_0^{\pi/2} \alpha(\theta)\,\big(2\sin\theta\cos\theta\big)\,d\theta.$$

The weight $2\sin\theta\cos\theta$ is the diffuse-field incidence distribution — the
same weighting the reverberation-chamber measurement implicitly applies.

---

## Step 3 — Substitute $\mu = \cos\theta$

Then $d\mu = -\sin\theta\,d\theta$, and the limits $\theta: 0 \to \tfrac{\pi}{2}$
become $\mu: 1 \to 0$. The weight $2\sin\theta\cos\theta\,d\theta$ becomes $2\mu\,d\mu$:

$$\alpha_{\text{random}}
  = \int_0^1 \frac{4\xi\mu}{(\xi\mu+1)^2}\,2\mu\,d\mu
  = 8\xi \int_0^1 \frac{\mu^2}{(\xi\mu+1)^2}\,d\mu.$$

---

## Step 4 — Evaluate the integral

Substitute $u = \xi\mu + 1$, so $\mu = \dfrac{u-1}{\xi}$ and $d\mu = \dfrac{du}{\xi}$.
The limits $\mu: 0 \to 1$ become $u: 1 \to \xi+1$:

$$\int_0^1 \frac{\mu^2}{(\xi\mu+1)^2}\,d\mu
  = \frac{1}{\xi^3}\int_1^{\xi+1} \frac{(u-1)^2}{u^2}\,du.$$

Expand the integrand:

$$\frac{(u-1)^2}{u^2} = 1 - \frac{2}{u} + \frac{1}{u^2}.$$

Integrate term by term:

$$\int_1^{\xi+1}\left(1 - \frac{2}{u} + \frac{1}{u^2}\right)du
  = \left[\,u - 2\ln u - \frac{1}{u}\,\right]_1^{\xi+1}.$$

At the lower limit $u = 1$: $\;1 - 0 - 1 = 0$. At the upper limit $u = \xi+1$:

$$(\xi+1) - 2\ln(\xi+1) - \frac{1}{\xi+1}.$$

---

## Step 5 — Assemble

$$\alpha_{\text{random}}
  = 8\xi \cdot \frac{1}{\xi^3}
    \left[(\xi+1) - 2\ln(\xi+1) - \frac{1}{\xi+1}\right]
  = \frac{8}{\xi^2}
    \left[(\xi+1) - \frac{1}{\xi+1} - 2\ln(\xi+1)\right].$$

---

## Step 6 — Tidy the rational part

$$(\xi+1) - \frac{1}{\xi+1}
  = \frac{(\xi+1)^2 - 1}{\xi+1}
  = \frac{\xi^2 + 2\xi}{\xi+1}
  = \frac{\xi(\xi+2)}{\xi+1}$$

giving the final closed form:

$$\boxed{\;\alpha_{\text{random}}(\xi)
  = \frac{8}{\xi^2}\left[\frac{\xi(\xi+2)}{\xi+1} - 2\ln(\xi+1)\right]\;}$$

---

## Sanity checks

- **Large $\xi$ (rigid wall):** the bracket $\to \xi$, so
  $\alpha_{\text{random}} \approx \dfrac{8}{\xi} \to 0$ — a perfect reflector absorbs
  nothing.
- **Peak:** the function maximises at $\xi \approx 1.567$ with
  $\alpha_{\text{random}} \approx 0.951$. This is the ceiling — a real,
  locally-reacting surface cannot exhibit random-incidence absorption above
  $\approx 0.95$, which is why calibration clamps targets above it.

---

## Plain-text form (for code / non-rendering viewers)

```
alpha(theta)   = 4*xi*cos(theta) / (xi*cos(theta) + 1)^2

alpha_random   = integral_0^{pi/2} alpha(theta) * 2*sin(theta)*cos(theta) d(theta)

               = (8 / xi^2) * [ xi*(xi + 2) / (xi + 1)  -  2*ln(xi + 1) ]
```
