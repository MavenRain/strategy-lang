# strategy-lang domain design

> Paste the domain design below the line, verbatim. Do not edit it.

This file is the only source of the core types and the core operations of
strategy-lang. `SPEC.md` sections 5 and 6 cite its sections. Do not add a core
type or a core operation that this file does not give.

When the design changes, paste the new version. Then record the change as an
open item in `SPEC.md` section 9.

---

I want to write a new programming language with the following features:  The base types and operation come from the minimal span of CGS + one-goal Strategy Logic: 

Concept,Type / Signature,Essential Operation
State,StStSt,—
Decision,Ag→AcAg\to AcAg→Ac,—
Transition,τ:St×(Ag→Ac)→St\tau:St\times(Ag\to Ac)\to Stτ:St×(Ag→Ac)→St,successor state
Track / Path,"Trk\mathsf{Trk}Trk, Pth\mathsf{Pth}Pth",history / outcome
Strategy,Trk⇀Ac\mathsf{Trk}\rightharpoonup AcTrk⇀Ac,action choice from history
Assignment,(Var∪Ag)⇀Str(\mathsf{Var}\cup Ag)\rightharpoonup\mathsf{Str}(Var∪Ag)⇀Str,bind variables/agents to strategies
Play,Asg×St→Pth\mathsf{Asg}\times St\to\mathsf{Pth}Asg×St→Pth,unique path induced by complete assignment
Principal sentence,"℘ ♭ ψ\wp\,\flat\,\psi℘♭ψ","quantify strategies, bind them, evaluate temporal goal"
Satisfaction,"G,χ,s⊨ϕG,\chi,s\models\phiG,χ,s⊨ϕ",inductive evaluation along plays

We also include the following type constructors: products, coproducts, monads (map/bind), algebras/coalgebras (fold/unfold), quotients (paths/loops/HIT/QIT from HoTT)

Proofs are not allowed to collapse due to irrelevance.

The host language shall be TinyCC, and the compilation output should be the JSON serialization of final states.
