# Plugin Architecture for phosphor-logging

Author:
  Matt Spinler <mspinler!>

Primary assignee:
  Matt Spinler

Other contributors:
  Brad Bishop

Created:
  April 29, 2019

## Problem Description
Companies that use OpenBMC may want to represent error logs in a format other
than the `xyz.openbmc_project.Logging.Entry` D-Bus interface provided by the
phosphor-logging code, while still allowing code to create errors using the
phosphor-logging API.

What is being proposed here to accomplish this is to add a plugin mechanism to
the phosphor-log-manager daemon that will allow others to plug in their own
event log code using their own shared libraries.  They are then free to create
their own event log objects however they want, possibly also in D-Bus, or maybe
just keeping them in the filesystem.

## Background and References
The `xyz.openbmc_project.Logging.Entry` interface represents an event log
entry.  The object paths that contain these intefaces are hosted by the
phosphor-log-manager daemon and are of the form:
`/xyz/openbmc_project/logging/entry/N`, where N is some number.  The API to
create a new event log is either calling phosphor::logging::elog() followed by
phosphor::logging::commit(), or a single call to phosphor::logging::report().

The event log objects also contain an `xyz.openbmc_project.Object.Delete`
interface to delete the event log that it's called on, and there's also an
`xyz.openbmc_project.Collection.DeleteAll` interface on a parent path that
allows all logs to be deleted at once.  The phosphor-log-manager daemon also
implements a simple algorithm for purging error logs when a new one is created
when it hits a build time specified limit.

## Requirements
1. There needs to be a way for other interested code to know when a new
   `xyz.openbmc_project.Logging.Entry` event has been created, and it needs
   access to the contents of that log.
1. The code needs to know both before and after an event log has been deleted.
1. There needs to be a way to override the default purging algorithm that may
   cause an old log to be deleted before a new one is added, so that a new
   algorithm can be used instead to know what logs to delete based on any of
   its own requirements.
1. There needs to be a spot for an initialization function to run when the
   phosphor-logging daemon starts.

## Proposed Design
This design adds the ability for shared libraries to be able to register
functions to call at certain points in the phosphor-logging code. It is similar
to how IPMI allows OEM handlers.

The phosphor-log-manager daemon will provide the following registration
functions in a globally available header file:

```
struct LogEntryParams
{
    uint32_t id;
    uint64_t timestamp;
    std::string severity;
    std::string message;
    bool resolved;
    std::vector<std::string> additionalData;
};

using preCreateCallback = std::function<void(const LogEntryParams& params)>;


/**
 * @brief Register a function to call before an event log is created.
 *
 * Called immediately before an event log object is created on D-Bus.
 *
 * @param[in] callback - The function to call.
 */
void registerPreCreationCallback(preCreateCallback callback);
```

```
using postCreateCallback = std::function<void(const LogEntryParams& params)>;

/**
 * @brief Register a function to call after an event log is created.
 *
 * Called immediately after an event log object is created on D-Bus.
 *
 * @param[in] callback - The function to call.
 */
void registerPostCreationCallback(postCreateCallback callback);
```

```
using preDeleteCallback = std::function<void(const LogEntryParams& params)>;

/**
 * @brief Register a function to call before an event is deleted.
 *
 * Called immediately before an event log object is deleted on D-Bus.
 *
 * @param[in] callback - The function to call.
 */
void registerPreDeletionCallback(preDeleteCallback callback);
```

```
using postDeleteCallback = std::function<void(const LogEntryParams& params)>;

/**
 * @brief Register a function to call after an event is deleted.
 *
 * Called immediately after an event log object is deleted on D-Bus.
 *
 * @param[in] callback - The function to call.
 */
void registerPostDeletionCallback(postDeleteCallback callback);
```

```
using ErrorIDs = std::vector<uint32_t>;
using purgeAlgorithmOverride = std::function<ErrorIDs()>;

/**
 * @brief Purge algorithm function to call instead of the default one.
 *
 * The callback function will return a list of event log IDs to tell
 * phosphor-log-manager what to delete.
 *
 * This is called before a new log is created to free up space for the new
 * log, if necessary.
 *
 * Only 1 of these functions may be registered at a time.  This is enforced
 * by the logging daemon as it will only accept the first one.
 * Alternatively it could assert if it detects more than one.
 *
 * @param[in] callback - The function to call.
 */
void registerPurgeAlgorithmOverride(purgeAlgorithmOverride override);
```

On startup, the logging daemon will dlopen any shared libraries it finds in the
`/usr/lib/logging-plugins/` directory.  The shared libraries will then
implement a constructor function (with `__attribute__((constructor))`) where
they can call these registration functions, and run any initilization code they
need to.  The logging daemon will keep track of all registered functions and
call them at the appropriate times.

Event log objects are persisted, so on start up, the plugins will get same
creation call after an event log is restored as when it was created.

If the plugin library needs to, it can also implement a destuctor function
identified by using using the `destructor` attribute which will be called
when dlclose runs when the logging daemon stops.

## Alternatives Considered
One alternative is to just create a new application that listens on D-Bus for
interfacesAdded and interfacesRemoved signals on the
`xyz.openbmc_project.Logging.Entry` interfaces and then just operates based on
that information.  The downside of this is it doesn't allow a way to override
the log purging algorithm that phosphor-logging uses.

## Impacts
This should be transparent to callers of phosphor-logging.

## Testing
Unit tests inside of phosphor-logging should be able to be added that have a
test plugin that runs and have its call points validated.
