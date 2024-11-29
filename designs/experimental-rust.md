# Experimental support for Rust in OpenBMC

Author: Peter Delevoryas (peter)

Other contributors: None

Created: Nov 27, 2024

## Problem Description

This document motivates the creation of a new repository that will be used as a
[Cargo workspace][1] for experimental Rust crates, which system implementors can
select in their OpenBMC images. This document does not argue that Rust should
be supported long-term by the OpenBMC project, rather, that we should create
an avenue for experimentation under the supervision of the OpenBMC project
maintainers.

## Background and References

[Rust 1.0][2] was released initially in 2015:

> Rust combines low-level control over performance with high-level convenience
> and safety guarantees.

Since then, it has become a widely adopted systems programming language, used as
an alternative to C, C++, and Java, but also as an alternative to higher-level
languages lacking static typing, like Python.

[rust-lang.org][3] has the latest details on its features and direction, but
it's important to note that [embedded systems][4] were added explicitly to
the roadmap in 2018, and there is a dedicated working-group associated with
improving embedded development in Rust.

Rust is a compiled language that uses LLVM for code generation and optimization,
and its efficiency and performance are [comparable to C and C++][5].

Some examples of projects people have used Rust for:

- [buck2][32], Meta's build system
- [eden][6], Meta's monorepo source control filesystem
- Several virtual machine managers: [firecracker][9], [cloud-hypervisor][8], [openvmm][11]
- [Hubris][13], [Oxide Computer Company][12]'s kernel for embedded devices,
- Meta's Training and Inference Accelerator ([MTIA][10]) userspace [driver][7]

There is now a growing interest in using Rust within OpenBMC developers.

Zev Weiss created [omnisensor][14], a rewrite of `dbus-sensors`, and posted
an analysis of Rust within OpenBMC to the mailing list [here][15]. He explains
that working on `dbus-sensors` has become tedious and difficult, and justifies
using Rust:

> ...it seems to me like the problems with the dbus-sensors codebase are broad
> enough and sufficiently inseparable that a serious effort to address them
> would amount to rewriting a very substantial fraction of its code. And if
> that's going to happen...why not take the plunge and do so in a better language?

Zev contrasts Rust with C++ in the post, and also explains that it's not
necessary to rewrite everything in Rust all-at-once: the two can co-exist
easily, to support incremental changes.

> ...aside from my general opinion that the current state of things with
> dbus-sensors is dire enough (and the benefits big enough) to merit \[rewriting\]
> in spite of the downsides, there's also the significant mitigating factor that
> the modularity of both dbus-sensors and omnisensor means that it wouldn't have
> to be an all-or-nothing transition. The two could easily coexist peacefully,
> managing disjoint subsets of a system's sensors according to each one's
> compile-time configuration (which dbus-sensors daemons are built and which
> omnisensor sensor-type backend features are enabled). A gradual transition
> would thus be easy to do one sensor-type/daemon at a time.

Later that year, at the 2023 OpenBMC Dev conference (FIXME: Was this a
mini-conference within OSFC?), some concerns about Rust were discussed
and recorded in a Google Doc [here][19]. To summarize the recorded concerns:

- Rust defaults to static linking, which may increase image size too much.
- Rust enables cargo-cult behavior: can we control the dependencies that applications pull in?
- What happens if multiple crates use different versions a dependency?
- How do users create Yocto bitbake recipes from Rust crates?
- Can Rust use the same generated D-Bus bindings that we're moving towards with C++?

At the end, it is suggested that to push Rust forward in OpenBMC, there should
be a design document proposal about resolving these concerns, so that the
technical oversight forum (TOF) can make a decision on the implications of
introducing Rust.

Lastly, a ["rust"][20] channel was created in the OpenBMC community Discord
server, where community members have shared links to summarize the above
information.

## Requirements

The following requirements describe what Rust, as an alternative programming
language, requires to be introduced into OpenBMC.

### D-Bus Support

In some projects, to introduce an alternative implementation language, foreign
function interface (FFI) bindings would be created to allow code from each
implementation language to call functions and access data in others. Usually
these bindings standardize on the C ABI as a common interface, but some
more-direct binding interfaces have appeared, like direct C++-to-Rust bindings.

Since OpenBMC is architected as a collection of system daemons communicating via
D-Bus interfaces, and because OpenBMC developers are more interested in writing
new daemons (as opposed to adding Rust to existing C++ daemons), FFI bindings
would *not* be appropriate. Instead, all Rust code should be written and compiled
separately from C++ code, without any dependencies dependencies on C++ objects.

With that in mind, Rust needs to support interacting with D-Bus services via
`dbus-broker`. This can either be a library that provides bindings to `libdbus`
or a pure-Rust implementation for interacting with `dbus-broker` to implement
most of the D-Bus API.

Creating and maintaining a new library for Rust D-Bus support would not be
out-of-the-question, because `sdbusplus` is already maintained within OpenBMC
for C++. However, this would be a large undertaking with a lot of risk for
failure.

### Target Platform Support

Rust is a compiled language, so the compiler needs to support the common OpenBMC
target platforms. The minimum viable support would be targeting the Aspeed
AST2620 (ARMv7-A), which is probably the most commonly used part being used in
new OpenBMC deployments where firmware developers are adding features today.
The AST2520 is probably the most widely deployed device in production, but there
is probably less active development for it. It's hard to quantify this without
surveying several different companies, but within Meta this is true. Generally,
support for ARMv7-A GNU/Linux environments would be important. ARMv8-A would be
important in the near future with the introduction of the AST2700 and Nuvoton
BMCs.

### Build System Support

OpenBMC uses Meson in all of its repositories to build C++, which supports:

- Building applications for common developer environments (usually
`x86_64-unknown-linux-gnu`)
- Running unit tests in common developer environments
- Building applications for target environments (e.g. such as
`armv7-unknown-linux-gnueabi`) using Yocto
- A system for using third-party libraries

Any new language introduced into OpenBMC should support similar workflows.
It is *not* required that the language can be built with Meson, just that
the build system can match the same workflows that developers use within Meson.

Building with Yocto *is* a requirement, and the Yocto integration should be in
upstream OpenEmbedded, not maintained as part of OpenBMC.

### Repository

A repository for daemons written in Rust is required. This repository should
support the build system selected, integration with Yocto, and be decoupled
from existing C++ repositories, except if a shared D-Bus interface definition
is used.

### Efficiency and Performance

New languages should have performance and efficiency comparable to C++.
Significant degradations in performance or efficiency would make components
in the alternate language difficult to achieve or maintain typical production
quality standards.

This includes the following dimensions:

- Size
- Speed
- CPU usage
- Memory usage

## Proposed Design

### D-Bus Support

[zbus][23] is a pure-Rust implementation of D-Bus serialization, the object
model, and a high-level [procedural macro][24] system for defining D-Bus
interfaces that generates easy-to-use Rust types and methods. Procedural macros
are commonly used in Rust to create libraries that transform data formats
specified by users as `struct` types. Some other examples include
[serde_json][25] (JSON serialization) and [clap][26] (CLI argument parser)

`zbus` supports synchronous and asynchronous APIs, so applications are not
restricted to using async runtimes like `tokio` to use `zbus`.

`zbus` should be preferred over other D-Bus libraries for its ease-of-use and
wide-usage within the Rust community unless D-Bus has been identified as a
bottleneck in an application. The amount of code written to interface with D-Bus
drops significantly: it's even simpler and easier than Python.

One other consideration with introducing another language is where we can share
the D-Bus interface definitions in `phosphor-dbus-interfaces` between C++ *and*
Rust code. A full design specification for how to implement this is out-of-scope
of this document, but it is reasonable to assume that we can auto-generate a
`zbus` library from the YAML files in `phosphor-dbus-interfaces`, or from D-Bus
XML using [zbus_xmlgen][36].

  // Hello world server

  use std::{error::Error, future::pending};
  use zbus::{connection, interface};

  struct Greeter {
      count: u64
  }

  #[interface(name = "org.zbus.MyGreeter1")]
  impl Greeter {
      // Can be `async` as well.
      fn say_hello(&mut self, name: &str) -> String {
          self.count += 1;
          format!("Hello {}! I have been called {} times.", name, self.count)
      }
  }

  // Although we use `tokio` here, you can use any async runtime of choice.
  #[tokio::main]
  async fn main() -> Result<(), Box<dyn Error>> {
      let greeter = Greeter { count: 0 };
      let _conn = connection::Builder::session()?
          .name("org.zbus.MyGreeter")?
          .serve_at("/org/zbus/MyGreeter", greeter)?
          .build()
          .await?;

      // Do other things or go to wait forever
      pending::<()>().await;

      Ok(())
  }

  // Hello world client

  use zbus::{Connection, Result, proxy};

  #[proxy(
      interface = "org.zbus.MyGreeter1",
      default_service = "org.zbus.MyGreeter",
      default_path = "/org/zbus/MyGreeter"
  )]
  trait MyGreeter {
      async fn say_hello(&self, name: &str) -> Result<String>;
  }

  // Although we use `tokio` here, you can use any async runtime of choice.
  #[tokio::main]
  async fn main() -> Result<()> {
      let connection = Connection::session().await?;

      // `proxy` macro creates `MyGreeterProxy` based on `Notifications` trait.
      let proxy = MyGreeterProxy::new(&connection).await?;
      let reply = proxy.say_hello("Maria").await?;
      println!("{reply}");

      Ok(())
  }

### Target Platform Support

Rust has different tiers of support for each target platform, listed [here][29].

`x86_64-unknown-linux-gnu` and `aarch64-unknown-linux-gnu` are tier 1 platforms,
which means Rust project developers run CI, build release binaries, and `rustc`
and `cargo` work natively on `x86_64` and `aarch64` GNU/Linux systems.

`arm-unknown-linux-gnueabi` is supported under "Tier 2 with Host Tools".

`armv7-unknown-linux-gnueabi` is supported under "Tier 2 without Host Tools",
with full standard library support.

These levels of support will likely guarantee that OpenBMC developers do not
need to worry about maintaining toolchain support other than reporting bugs
occasionally.

### Build System Support

Cargo should be used to build Rust code in OpenBMC, because it has the best
support for building Rust projects, and a Yocto layer called
[meta-rust][30] has already been developed to provide support for building
Cargo projects, including generating bitbake `.bb` recipes from `Cargo.toml`
files. Cargo supports generating and running unit tests annotated with `#[test]`
via `cargo test`, and has an extensive third-party package repository hosted on
[crates.io][33], as well as support for referencing git repositories directly.

### Efficiency and Performance

Rust has been characterized widely as a drop-in replacement for C++, but refer
to ["Towards Understanding the Runtime Performance of Rust"][5] for a detailed
examination. Even naive Rust, without significant attention to detail or
micro-optimization, should behave similarly to naive C++. This characterization
refers to "release" (optimized) builds of Rust applications though: in
some cases, unoptimized Rust may be significantly larger than similar C++
applications.

Rust executables default to being built statically, which may result in larger
executable sizes. This can be mitigated by building executables with the `-C
prefer-dynamic` option. At one point earlier in its history, `-C prefer-dynamic`
was the default. The `meta-rust` Yocto layer does this by default after [this][35]
change.

Other tricks like enabling link-time optimization may be more important in Rust
to strip dead code, but can be easily configured at compile-time.

## Alternatives Considered

(2 paragraphs) Include alternate design ideas here which you are leaning away
from. Elaborate on why a design was considered and why the idea was rejected.
Show that you did an extensive survey about the state of the art. Compares your
proposal's features & limitations to existing or similar solutions.

### D-Bus Support

Rust has 2 major D-Bus API libraries: [dbus-rs][22] and **[zbus][23]**.

`dbus-rs` provides bindings to `libdbus`, which is the C API for D-Bus. It is
efficient, but not very easy to use.

Both `dbus-rs` and `zbus` support synchronous and asynchronous programming,
including integrations with the most commonly used Rust async runtime `tokio`.

Zev used `dbus-rs` in `omnisensor`, because of some efficiency issues he noticed
after [benchmarking][27] several libraries against one another on an Aspeed
AST2520:

  perf: client/server cross-product between sd_bus, sdbusplus, dbus-rs, zbus
   - property get/set, empty method call

  #server client time

  500 calls:
                   (clients)
  (servers)  sdbus  sdbusplus  dbusrs  zbus
  sdbus      2.028    2.041    2.551   4.266
  sdbusplus  2.144    2.140    2.648   4.308
  dbusrs     1.663    1.669    2.195   3.870
  zbus       4.504    4.500    5.029   6.812

  500 gets:
                   (clients)
  (servers)  sdbus  sdbusplus  dbusrs  zbus
  sdbus      1.408    1.397    2.087   0.041
  sdbusplus  1.471    1.485    2.137   0.036
  dbusrs     1.896    1.900    2.547   0.038
  zbus       5.704    5.690    6.361   0.043

  500 sets:
                   (clients)
  (servers)  sdbus  sdbusplus  dbusrs  zbus
  sdbus      2.175    2.165    2.910   4.536
  sdbusplus  2.273    2.244    3.008   4.750
  dbusrs     2.550    2.533    3.320   5.000
  zbus       7.275    7.224    8.006   9.784

With the following versions of the Rust libraries:

  tokio = { version = "1.22", features = ["rt", "macros"], default-features = false }
  dbus = "~0.9.7"
  dbus-tokio = "~0.7.5"
  dbus-crossroads = "~0.5.2"
  futures = { default-features = false, version = "0.3" }
  zbus = { version = "3.14", default-features = false, features = ["tokio"] }

However, in [zbus 5.0][28], some improvements to serialization have resulted
in performance improvements up-to 94%, so these benchmarks need to be retaken
for an accurate comparison.

  ⚡️ Replace Type::signature method with a const named SIGNATURE. This greatly
  improves the (de)serialization performance, in some cases even up by 94%.
  ⚡️ Don't check strings for embedded null bytes. In most cases (I'd say at
  least 99.9% of them), this is needless. They've a non-zero cost, which is high
  enough to be noticeable in case of a lot of strings being serialized.
  ⚡️ Micro optimization of byte arrays.

Furthermore, if there are still any performance issues, we should be able to
report them to the `zbus` maintainer Zeeshan and work with him to resolve the
gaps: he is very active, and eager to resolve any issues.

Because OpenBMC daemons typically only have soft-realtime latency requirements,
and these are strictly micro-benchmarking results of D-Bus interactions,
ease-of-use should be prioritized.

### Build System Support - Meson

Meson "supports" Rust, so it would be natural to consider building Rust daemons
alongside C++ ones with Meson, but this would probably introduce a lot of
friction.

Meson support builds Rust with the Rust compiler `rustc`, but does not integrate
with Cargo in a first-class way. This might be fine if you limit your project
to dependencies without dependencies (no transitive dependencies), but there are
very few useful crates like that. Refer to [this][16] issue for more details.

Meson 1.3.0 has [experimental][31] support for generating subprojects
`.wrap` files from `Cargo.toml` files, but the OpenBMC project is currently
only requiring `meson` 1.1.1. So we would at least need to require 1.3.0 in
repositories using Rust.

Similar to other Cargo translation systems like [Reindeer][21] (used to generate
`BUCK` files), some crates rely on build scripts or system dependencies that
cannot be easily auto-translated, requiring the non-Cargo user to write and
carry fixups of some kind.

If the `meta-rust` Yocto layer didn't exist, and there was no way to build Cargo
projects within Yocto, then Meson would be considered more strongly.

Similarly, if OpenBMC was not architected around D-Bus interfaces, and FFI
bindings were necessary to create a heterogeneous C++/Rust executable, then
Meson would be considered more strongly.

However, since a Yocto layer exists, the OpenBMC project is architected around
many small repositories, and D-Bus interfaces are used to communicate between
components, it's natural to just use Cargo in a separate repository.

### Repository

A single new repository should be created, consisting of a [Cargo workspace][1]
that can house multiple Rust packages, known as "crates". Each crate can provide
a separate set of executables, libraries, and tests. Crates within the workspace
can directly depend on each other via relative filesystem paths. All of the
code in this repository would be built with Cargo exclusively. The name should
be something like `phosphor-rust`, and does not need to be tied to any specific
domain of OpenBMC initially. If the repository grows past the experimental
stage, it can be split into multiple domain-specific repositories.

### Efficiency and Performance

One interesting examination of Rust binary size is
[Writing a winning 4K into in Rust][34], which reviews the changes necessary to
translate a size-optimized C program to Rust.

Some of these include replacing normal array or `Vec` index operator `[]`
accesses with `.get_unchecked()` method calls, to eliminate bounds-checking,
or replacing idiomatic `for x in 0..10 {...}` loops with more verbose do-while
`loop {...}` statements, avoiding bloat related to Rust iterators.
These classes of micro-optimization are unlikely to be necessary to achieve the
size or speed requirements of OpenBMC applications though.

## Impacts

Enabling Rust development as part of the upstream OpenBMC project will enable
community members to experiment with developing and pushing Rust into production
OpenBMC images, and may enable higher productivity. It should also make it
possible for the technical oversight forum (TOF) to make a determination on the
future of Rust within OpenBMC as an officially supported language, subject to
further design reviews.

Introducing another implementation language will also enforce adherence to D-Bus
interfaces more strongly: daemons that manage state strictly within C++ will not
be able to interact with any Rust daemons that are created, and vice-versa.

### Organizational

- Does this repository require a new repository? (Yes)
- Who will be the initial maintainer(s) of this repository? (@stwcx)
FIXME: Can @peter volunteer?
- Which repositories are expected to be modified to execute this design? None

## Testing

Testing can be done similarly to Meson workflows: Use `cargo build` to build
Rust, and `cargo test` to run unit tests. This will require updating the
CI testing workflows to be aware of the distinction between Meson vs Cargo
repositories within OpenBMC.

[1]: https://doc.rust-lang.org/book/ch14-03-cargo-workspaces.html
[2]: https://blog.rust-lang.org/2015/05/15/Rust-1.0.html
[3]: https://www.rust-lang.org/
[4]: https://www.rust-lang.org/what/embedded
[5]: https://dl.acm.org/doi/pdf/10.1145/3551349.3559494
[6]: https://github.com/facebook/sapling/blob/main/eden/fs/docs/Overview.md
[7]: https://youtu.be/WBfVHifgnig?si=-3Dv_nT57mvoNce9
[8]: https://github.com/cloud-hypervisor/cloud-hypervisor
[9]: https://github.com/firecracker-microvm/firecracker
[10]: https://ai.meta.com/blog/next-generation-meta-training-inference-accelerator-AI-MTIA/
[11]: https://github.com/microsoft/openvmm
[12]: https://oxide.computer/
[13]: https://github.com/oxidecomputer/hubris
[14]: https://github.com/zevweiss/omnisensor
[15]: https://lore.kernel.org/openbmc/Y79U52toP0+Y4edh@hatter.bewilderbeest.net/
[16]: https://github.com/mesonbuild/meson/issues/2173
[17]: https://mesonbuild.com/Wrap-dependency-system-manual.html#cargo-wraps
[18]: https://doc.rust-lang.org/book/ch14-03-cargo-workspaces.html
[19]: https://docs.google.com/document/d/1PRvFbUo4Oup9uV4OuxxwRrUpNl3T1JqZB9rCkg4h8Gk/edit?tab=t.0#heading=h.czyyvz5z64dh
[20]: https://discord.com/channels/775381525260664832/1255343125799374928
[21]: https://github.com/facebookincubator/reindeer
[22]: https://github.com/diwic/dbus-rs
[23]: https://github.com/dbus2/zbus
[24]: https://doc.rust-lang.org/reference/procedural-macros.html
[25]: https://docs.rs/serde_json/latest/serde_json/
[26]: https://docs.rs/clap/latest/clap/_derive/_tutorial/chapter_0/index.html
[27]: https://github.com/zevweiss/obmc-dbus-benchmarks
[28]: https://github.com/dbus2/zbus/releases/tag/zvariant-5.0.0
[29]: https://doc.rust-lang.org/nightly/rustc/platform-support.html
[30]: https://github.com/meta-rust/meta-rust
[31]: https://mesonbuild.com/Wrap-dependency-system-manual.html#cargo-wraps
[32]: https://github.com/facebook/buck2
[33]: https://crates.io/
[34]: https://www.codeslow.com/2020/07/writing-winning-4k-intro-in-rust.html
[35]: https://github.com/meta-rust/meta-rust/commit/8a6f084c66fd93b983d10b52fa3c8d596f8a5280
[36]: https://github.com/dbus2/zbus/tree/main/zbus_xmlgen
