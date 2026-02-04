# NOTES ON DEFAULT PARAMETER VALUES

## Entrance Net properties

The following values were derived from figures presented in Sonter et al. _Protective covers impact honey bee colony performance and access to outside resources_, Agriculture, Ecosystems and Environment 368, 2024 (Romina Rader et al). The derivation of the values used below was aided by ChatGPT in a chat labelled "Paper Summary: Honey Bees" on 3/2/26.

### Per-Attempt Exit Success Probability Calculations

The paper reports, for each net type:

- an **overall probability of exit** \(S\)
- a **mean number of rebounds** \(\bar R\)

A rebound is interpreted as a **failed exit attempt**.
Thus, the mean number of exit attempts is:

\[
\bar A \approx \bar R + 1
\]

### Modelling Assumption

Assume:
- Each exit attempt is independent
- Each attempt has constant success probability \(p\)
- A bee succeeds if at least one attempt succeeds

Then the probability of exiting after \(\bar A\) attempts is:

\[
S = 1 - (1 - p)^{\bar A}
\]

Solving for \(p\):

\[
p = 1 - (1 - S)^{1/\bar A}
\]

### netAntihailExitProb // probability of bee exiting through anti-hail net

Default = 0.0371

From the paper:
- \(S_{\text{hail}} = 0.3146\)
- \(\bar R_{\text{hail}} = 8.98\)

Mean attempts:
\[
\bar A_{\text{hail}} = 8.98 + 1 = 9.98
\]

Per-attempt success probability:
\[
p_{\text{hail}}
= 1 - (1 - 0.3146)^{1/9.98}
= 1 - 0.6854^{1/9.98}
\approx 0.037
\]

\[
\boxed{p_{\text{hail}} \approx 3.7\% \text{ per attempt}}
\]

### netAntibirdExitProb // probability of bee exiting through anti-bird net

Default = 0.1187

From the paper:
- \(S_{\text{bird}} = 0.5073\)
- \(\bar R_{\text{bird}} = 4.60\)

Mean attempts:
\[
\bar A_{\text{bird}} = 4.60 + 1 = 5.60
\]

Per-attempt success probability:
\[
p_{\text{bird}}
= 1 - (1 - 0.5073)^{1/5.60}
= 1 - 0.4927^{1/5.60}
\approx 0.119
\]

\[
\boxed{p_{\text{bird}} \approx 11.9\% \text{ per attempt}}
\]

### int netAntibirdMaxExitAttempts // max exit attempts through anti-bird net

Default: 7

### int netAntihailMaxExitAttempts // max exit attempts through anti-hail net

Default: 11

### Justification & calculations for choosing \(k_{\max,\text{bird}}=7\) and \(k_{\max,\text{hail}}=11\)

This derivation uses a **truncated geometric “attempts” model** to connect the paper’s:
- overall exit success probability \(S\), and
- mean number of rebounds \(\bar R\)

to a plausible **maximum number of attempts** \(k_{\max}\).
(All empirical values come from Sonter et al. 2024.) :contentReference[oaicite:0]{index=0}

---

#### 1) Behavioural model (assumption)

For a given net type, assume:

1. Each contact with the net corresponds to an **exit attempt**.
2. Each attempt succeeds with constant probability \(p\) (independent across attempts).
3. A bee will try at most \(k_{\max}\) attempts.
4. If the bee succeeds on attempt \(j\), it has made \(j-1\) **rebounds** (failures).
5. If the bee fails all \(k_{\max}\) attempts, it registers \(k_{\max}\) rebounds.

Under this model:

##### Overall probability of exiting
\[
S \;=\; 1 - (1-p)^{k_{\max}}
\]
so for any candidate \(k_{\max}\):
\[
p(k_{\max}) \;=\; 1 - (1-S)^{1/k_{\max}}
\]

##### Expected rebounds (including the “all-fail” case)
Let \(q = 1-p\). Then
\[
\mathbb{E}[R]
\;=\;
\sum_{j=1}^{k_{\max}} (j-1)\,q^{\,j-1}p
\;+\;
k_{\max}\,q^{\,k_{\max}}
\]

---

#### 2) Fitting \(k_{\max}\) from the paper’s \((S,\bar R)\)

**Procedure:**
1. Pick an integer candidate \(k_{\max}\).
2. Compute \(p\) from the observed \(S\):
   \[
   p = 1 - (1-S)^{1/k_{\max}}
   \]
3. Compute \(\mathbb{E}[R]\) from the formula above.
4. Choose the \(k_{\max}\) that makes \(\mathbb{E}[R]\) closest to the observed mean \(\bar R\).

This is effectively a **1D integer search** over plausible \(k_{\max}\) values.

---

#### 3) Anti-hail netting: why \(k_{\max,\text{hail}}=11\)

From the paper (anti-hail):
\[
S_{\text{hail}} = 0.3146,\quad \bar R_{\text{hail}} = 8.98
\]
:contentReference[oaicite:1]{index=1}

Try \(k_{\max}=11\):

##### Step A — compute per-attempt success probability
\[
p_{\text{hail}}(11)
= 1 - (1-0.3146)^{1/11}
= 1 - 0.6854^{1/11}
\approx 0.03376
\]

So \(q \approx 0.96624\).

##### Step B — compute expected rebounds
\[
\mathbb{E}[R]_{\text{hail}}(11)
=
\sum_{j=1}^{11} (j-1)\,q^{\,j-1}p
+
11\,q^{11}
\approx 9.0046
\]

Comparison:
\[
9.0046 \text{ vs } \bar R_{\text{hail}} = 8.98
\quad(\text{difference } \approx 0.025)
\]

This is a very close match.
Nearby values fit worse (e.g., \(k_{\max}=10 \Rightarrow \mathbb{E}[R]\approx 8.1719\); \(k_{\max}=12 \Rightarrow \mathbb{E}[R]\approx 9.8374\)).

**Hence \(k_{\max,\text{hail}}=11\)** is a plausible integer choice under this model.

---

#### 4) Anti-bird netting: why \(k_{\max,\text{bird}}=7\)

From the paper (anti-bird):
\[
S_{\text{bird}} = 0.5073,\quad \bar R_{\text{bird}} = 4.60
\]
:contentReference[oaicite:2]{index=2}

Try \(k_{\max}=7\):

##### Step A — compute per-attempt success probability
\[
p_{\text{bird}}(7)
= 1 - (1-0.5073)^{1/7}
= 1 - 0.4927^{1/7}
\approx 0.09618
\]

So \(q \approx 0.90382\).

##### Step B — compute expected rebounds
\[
\mathbb{E}[R]_{\text{bird}}(7)
=
\sum_{j=1}^{7} (j-1)\,q^{\,j-1}p
+
7\,q^{7}
\approx 4.7673
\]

Comparison:
\[
4.7673 \text{ vs } \bar R_{\text{bird}} = 4.60
\quad(\text{difference } \approx 0.167)
\]

This is the closest fit among nearby integers (e.g., \(k_{\max}=6 \Rightarrow \mathbb{E}[R]\approx 4.0514\); \(k_{\max}=8 \Rightarrow \mathbb{E}[R]\approx 5.4835\)).

**Hence \(k_{\max,\text{bird}}=7\)** is a reasonable integer choice under this model.

---

#### 5) Notes / limitations

- The paper reports **means** (\(S\) and \(\bar R\)), not full distributions.
  Therefore \(k_{\max}\) is **not uniquely identifiable** without assuming a specific decision model.
- The truncated geometric assumption is a clean, practical model for ABMs:
  it yields a \(k_{\max}\) consistent with both success rate and mean rebounds,
  and lets net type differences express themselves primarily through \(p\) and/or \(k_{\max}\).

---
