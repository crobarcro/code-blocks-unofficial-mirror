/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU Lesser General Public License, version 3
 * http://www.gnu.org/licenses/lgpl-3.0.html
 */

#ifndef CBPLUGIN_H
#define CBPLUGIN_H

#include <wx/arrstr.h>
#include <wx/event.h>
#include <wx/intl.h>
#include <wx/string.h>

#include "settings.h" // build settings
#include "globals.h"
#include "logger.h"
#include "manager.h"
#include "pluginmanager.h"
#include "prep.h"

#ifdef __WXMSW__
    #ifndef PLUGIN_EXPORT
        #ifdef EXPORT_LIB
            #define PLUGIN_EXPORT __declspec (dllexport)
        #else // !EXPORT_LIB
            #ifdef BUILDING_PLUGIN
                #define PLUGIN_EXPORT __declspec (dllexport)
            #else // !BUILDING_PLUGIN
                #define PLUGIN_EXPORT __declspec (dllimport)
            #endif // BUILDING_PLUGIN
        #endif // EXPORT_LIB
    #endif // PLUGIN_EXPORT
#else
    #define PLUGIN_EXPORT
#endif

// this is the plugins SDK version number
// it will change when the SDK interface breaks
#define PLUGIN_SDK_VERSION_MAJOR   2
#define PLUGIN_SDK_VERSION_MINOR   25
#define PLUGIN_SDK_VERSION_RELEASE 0

// class decls
class wxMenuBar;
class wxMenu;
class wxToolBar;
class wxPanel;
class wxWindow;

class cbBreakpoint;
class cbConfigurationPanel;
class cbDebuggerConfiguration;
class cbEditor;
class cbProject;
class cbStackFrame;
class cbStatusBar;
class cbThread;
class cbWatch;
class Compiler;
class CompileTargetBase;
class ConfigManagerWrapper;
class FileTreeData;
class ProjectBuildTarget;

struct PluginInfo;

// Define basic groups for plugins' configuration.
static const int cgCompiler         = 0x01; ///< Compiler related.
static const int cgEditor           = 0x02; ///< Editor related.
static const int cgCorePlugin       = 0x04; ///< One of the core plugins.
static const int cgContribPlugin    = 0x08; ///< One of the contrib plugins (or any third-party plugin for that matter).
static const int cgUnknown          = 0x10; ///< Unknown. This will be probably grouped with cgContribPlugin.

/** @brief Base class for plugins
  *
  * This is the most basic class a plugin must descend
  * from.
  * cbPlugin descends from wxEvtHandler, so it provides its methods as well...
  * \n \n
  * It's not enough to create a new plugin. You must also provide a resource
  * zip file containing a file named "manifest.xml". Check the manifest.xml
  * file of existing plugins to see how to create one (it's ultra-simple).
  */
class PLUGIN_EXPORT cbPlugin : public wxEvtHandler
{
    public:
        /** In default cbPlugin's constructor the associated PluginInfo structure
          * is filled with default values. If you inherit from cbPlugin, you
          * should fill the m_PluginInfo members with the appropriate values.
          */
        cbPlugin();

        /** cbPlugin destructor. */
        ~cbPlugin() override;

        /** The plugin must return its type on request. */
        virtual PluginType GetType() const { return m_Type; }

        /** Return the plugin's configuration priority.
          * This is a number (default is 50) that is used to sort plugins
          * in configuration dialogs. Lower numbers mean the plugin's
          * configuration is put higher in the list.
          */
        virtual int GetConfigurationPriority() const { return 50; }

        /** Return the configuration group for this plugin. Default is cgUnknown.
          * Notice that you can logically AND more than one configuration groups,
          * so you could set it, for example, as "cgCompiler | cgContribPlugin".
          */
        virtual int GetConfigurationGroup() const { return cgUnknown; }

        /** Return plugin's configuration panel.
          * @param parent The parent window.
          * @return A pointer to the plugin's cbConfigurationPanel. It is deleted by the caller.
          */
        virtual cbConfigurationPanel* GetConfigurationPanel(cb_optional wxWindow* parent) { return nullptr; }
        virtual cbConfigurationPanel* GetConfigurationPanelEx(wxWindow* parent,
                                                              cb_optional cbConfigurationPanelColoursInterface *colourInterface)
        {
            return GetConfigurationPanel(parent);
        }

        /** Return plugin's configuration panel for projects.
          * The panel returned from this function will be added in the project's
          * configuration dialog.
          * @param parent The parent window.
          * @param project The project that is being edited.
          * @return A pointer to the plugin's cbConfigurationPanel. It is deleted by the caller.
          */
        virtual cbConfigurationPanel* GetProjectConfigurationPanel(cb_optional wxWindow* parent, cb_optional cbProject* project) { return nullptr; }

        /** This method is called by Code::Blocks and is used by the plugin
          * to add any menu items it needs on Code::Blocks's menu bar.\n
          * It is a pure virtual method that needs to be implemented by all
          * plugins. If the plugin does not need to add items on the menu,
          * just do nothing ;)
          *
          * @note This function may be called more than one time. This can happen,
          * for example, when a plugin is installed or uninstalled.
          *
          * @param menuBar the wxMenuBar to create items in
          */
        virtual void BuildMenu(cb_optional wxMenuBar* menuBar) {}

        /** This method is called by Code::Blocks core modules (EditorManager,
          * ProjectManager etc) and is used by the plugin to add any menu
          * items it needs in the module's popup menu. For example, when
          * the user right-clicks on a project file in the project tree,
          * ProjectManager prepares a popup menu to display with context
          * sensitive options for that file. Before it displays this popup
          * menu, it asks all attached plugins (by asking PluginManager to call
          * this method), if they need to add any entries
          * in that menu. This method is called.\n
          * If the plugin does not need to add items in the menu,
          * just do nothing ;)
          * @param type the module that's preparing a popup menu
          * @param menu pointer to the popup menu
          * @param data pointer to FileTreeData object (to access/modify the file tree)
          */
        virtual void BuildModuleMenu(cb_optional const ModuleType type, cb_optional wxMenu* menu, cb_optional const FileTreeData* data = nullptr) { }

        /** This method is called by Code::Blocks and is used by the plugin
          * to add any toolbar items it needs on Code::Blocks's toolbar.\n
          * It is a pure virtual method that needs to be implemented by all
          * plugins. If the plugin does not need to add items on the toolbar,
          * just do nothing ;)
          * @param toolBar the wxToolBar to create items on
          * @return The plugin should return true if it needed the toolbar, false if not
          */
        virtual bool BuildToolBar(cb_optional wxToolBar* toolBar ) { return false; }

        /** This method return the priority of the plugin's toolbar, the less value
          * indicates a more preceding position when C::B starts with no configuration file
          */
        virtual int GetToolBarPriority() { return 50; }

#if wxUSE_STATUSBAR
        /** This method is called by Code::Blocks and is used by the plugin
          * to add a field on Code::Blocks's statusbar.\n
          * If the plugin does not need to add items on the statusbar, just
          * do nothing ;)
          * @param statusBar the cbStatusBar to create items on
          */
        virtual void CreateStatusField(cbStatusBar *statusBar) { wxUnusedVar(statusBar); }
#endif

        /** See whether this plugin is attached or not. A plugin should not perform
          * any of its tasks, if not attached...
          * @note This function is *not* virtual.
          * @return Returns true if it attached, false if not.
          */
        bool IsAttached() const { return m_IsAttached; }

        /** See whether this plugin can be detached (unloaded) or not.
          * This function is called usually when the user requests to
          * uninstall or disable a plugin. Before disabling/uninstalling it, Code::Blocks
          * asks the plugin if it can be detached or not. In other words, it checks
          * to see if it can be disabled/uninstalled safely...
          * @par
          * A plugin should return true if it can be detached at this moment, false if not.
          * @return The default implementation returns true.
          */
        virtual bool CanDetach() const { return true; }
    protected:
        /** Any descendent plugin should override this virtual method and
          * perform any necessary initialization. This method is called by
          * Code::Blocks (PluginManager actually) when the plugin has been
          * loaded and should attach in Code::Blocks. When Code::Blocks
          * starts up, it finds and <em>loads</em> all plugins but <em>does
          * not</em> activate (attaches) them. It then activates all plugins
          * that the user has selected to be activated on start-up.\n
          * This means that a plugin might be loaded but <b>not</b> activated...\n
          * Think of this method as the actual constructor...
          */
        virtual void OnAttach(){}

        /** Any descendent plugin should override this virtual method and
          * perform any necessary de-initialization. This method is called by
          * Code::Blocks (PluginManager actually) when the plugin has been
          * loaded, attached and should de-attach from Code::Blocks.\n
          * Think of this method as the actual destructor...
          * @param appShutDown If true, the application is shutting down. In this
          *         case *don't* use Manager::Get()->Get...() functions or the
          *         behaviour is undefined...
          */
        virtual void OnRelease(cb_optional bool appShutDown){}

        /** This method logs a "Not implemented" message and is provided for
          * convenience only.
          */
        virtual void NotImplemented(const wxString& log) const;

        /** Holds the plugin's type. Set in the default constructor. */
        PluginType m_Type;

        /** Holds the "attached" state. */
        bool m_IsAttached;

    private:
        friend class PluginManager; // only the plugin manager has access here

        /** Attach is <b>not</b> a virtual function, so you can't override it.
          * The default implementation hooks the plugin to Code::Block's
          * event handling system, so that the plugin can receive (and process)
          * events from Code::Blocks core library. Use OnAttach() for any
          * initialization specific tasks.
          * @see OnAttach()
          */
        void Attach();

        /** Release is <b>not</b> a virtual function, so you can't override it.
          * The default implementation un-hooks the plugin from Code::Blocks's
          * event handling system. Use OnRelease() for any clean-up specific
          * tasks.
          * @param appShutDown If true, the application is shutting down. In this
          *         case *don't* use Manager::Get()->Get...() functions or the
          *         behaviour is undefined...
          * @see OnRelease()
          */
        void Release(bool appShutDown);
};

/** @brief Base class for compiler plugins
  *
  * This plugin type must offer some pre-defined build facilities, on top
  * of the generic plugin's.
  */
class PLUGIN_EXPORT cbCompilerPlugin: public cbPlugin
{
    public:
        cbCompilerPlugin();

        /** @brief Run the project/target.
          *
          * Running a project means executing its build output. Of course
          * this depends on the selected build target and its type.
          *
          * @param target The specific build target to "run". If NULL, the plugin
          * should ask the user which target to "run" (except maybe if there is
          * only one build target in the project).
          */
        virtual int Run(ProjectBuildTarget* target = nullptr) = 0;

        /** Same as Run(ProjectBuildTarget*) but with a wxString argument. */
        virtual int Run(const wxString& target) = 0;

        /** @brief Clean the project/target.
          *
          * Cleaning a project means deleting any files created by building it.
          * This includes any object files, the binary output file, etc.
          *
          * @param target The specific build target to "clean". If NULL, it
          * cleans all the build targets of the current project.
          */
        virtual int Clean(ProjectBuildTarget* target = nullptr) = 0;

        /** Same as Clean(ProjectBuildTarget*) but with a wxString argument. */
        virtual int Clean(const wxString& target) = 0;

        /** @brief DistClean the project/target.
          *
          * DistClean will typically remove any config files
          * and anything else that got created as part of
          * building a software package.
          *
          * @param target The specific build target to "distclean". If NULL, it
          * cleans all the build targets of the current project.
          */
        virtual int DistClean(ProjectBuildTarget* target = nullptr) = 0;

        /** Same as DistClean(ProjectBuildTarget*) but with a wxString argument. */
        virtual int DistClean(const wxString& target) = 0;

        /** @brief Build the project/target.
          *
          * @param target The specific build target to build. If NULL, it
          * builds all the targets of the current project.
          */
        virtual int Build(ProjectBuildTarget* target = nullptr) = 0;

        /** Same as Build(ProjectBuildTarget*) but with a wxString argument. */
        virtual int Build(const wxString& target) = 0;

        /** @brief Rebuild the project/target.
          *
          * Rebuilding a project is equal to calling Clean() and then Build().
          * This makes sure that all compilable files in the project will be
          * compiled again.
          *
          * @param target The specific build target to rebuild. If NULL, it
          * rebuilds all the build targets of the current project.
          */
        virtual int Rebuild(ProjectBuildTarget* target = nullptr) = 0;

        /** Same as Rebuild(ProjectBuildTarget*) but with a wxString argument. */
        virtual int Rebuild(const wxString& target) = 0;

        /** @brief Build all open projects.
          * @param target If not empty, the target to build in each project. Else all targets.
          */
        virtual int BuildWorkspace(const wxString& target = wxEmptyString) = 0;

        /** @brief Rebuild all open projects.
          * @param target If not empty, the target to rebuild in each project. Else all targets.
          */
        virtual int RebuildWorkspace(const wxString& target = wxEmptyString) = 0;

        /** @brief Clean all open projects.
          * @param target If not empty, the target to clean in each project. Else all targets.
          */
        virtual int CleanWorkspace(const wxString& target = wxEmptyString) = 0;

        /** @brief Compile a specific file.
          *
          * @param file The file to compile (must be a project file!)
          */
        virtual int CompileFile(const wxString& file) = 0;

        /** @brief Abort the current build process. */
        virtual int KillProcess() = 0;

        /** @brief Is the plugin currently compiling? */
        virtual bool IsRunning() const = 0;

        /** @brief Get the exit code of the last build process. */
        virtual int GetExitCode() const = 0;

        /** @brief Display configuration dialog.
          *
          * @param project The selected project (can be NULL).
          * @param target The selected target (can be NULL).
          */
        virtual int Configure(cbProject* project, ProjectBuildTarget* target, wxWindow *parent) = 0;
    private:
};


class wxScintillaEvent;

struct cbDebuggerFeature
{
    enum Flags
    {
        Breakpoints,
        Callstack,
        CPURegisters,
        Disassembly,
        ExamineMemory,
        Threads,
        Watches,
        ValueTooltips,
        RunToCursor,
        SetNextStatement
    };
};

/** @brief Base class for debugger plugins
  *
  * This plugin type must offer some pre-defined debug facilities, on top
  * of the generic plugin's.
  */
class PLUGIN_EXPORT cbDebuggerPlugin: public cbPlugin
{
    public:
        cbDebuggerPlugin(const wxString& guiName, const wxString& settingsName);

    public:
        void OnAttach() override;
        void OnRelease(bool appShutDown) override;

        void BuildMenu(wxMenuBar* menuBar) override;
        void BuildModuleMenu(const ModuleType type, wxMenu* menu, const FileTreeData* data = nullptr) override;
        bool BuildToolBar(wxToolBar* toolBar) override;

        /** @brief Notify the debugger that lines were added or removed in an editor.
          * This causes the debugger to keep the breakpoints list in-sync with the
          * editors (i.e. what the user sees).
          * @param editor The editor in question.
          * @param startline The starting line this change took place.
          * @param lines The number of lines added or removed. If it's a positive number,
          *              lines were added. If it's a negative number, lines were removed.
          */
        virtual void EditorLinesAddedOrRemoved(cbEditor* editor, int startline, int lines);
    public:
        virtual void OnAttachReal() = 0;
        virtual void OnReleaseReal(bool appShutDown) = 0;

        virtual void SetupToolsMenu(wxMenu &menu) = 0;
        virtual bool ToolMenuEnabled() const;

        virtual bool SupportsFeature(cbDebuggerFeature::Flags flag) = 0;

        virtual cbDebuggerConfiguration* LoadConfig(const ConfigManagerWrapper &config) = 0;

        cbDebuggerConfiguration& GetActiveConfig();
        void SetActiveConfig(int index);
        int GetIndexOfActiveConfig() const;

        /** @brief Called when the user clicks OK in Settings -> Debugger... */
        virtual void OnConfigurationChange(bool isActive) { wxUnusedVar(isActive); };

        /** @brief Start a new debugging process. */
        virtual bool Debug(bool breakOnEntry) = 0;

        /** @brief Continue running the debugged program. */
        virtual void Continue() = 0;

        /** @brief Run the debugged program until it reaches the cursor at the current editor */
        virtual bool RunToCursor(const wxString& filename, int line, const wxString& line_text) = 0;

        /** @brief Sets the position of the Program counter to the specified filename:line */
        virtual void SetNextStatement(const wxString& filename, int line) = 0;

        /** @brief Execute the next instruction and return control to the debugger. */
        virtual void Next() = 0;

        /** @brief Execute the next instruction and return control to the debugger. */
        virtual void NextInstruction() = 0;

        /** @brief Execute the next instruction and return control to the debugger, if the instruction is a function call step into it. */
        virtual void StepIntoInstruction() = 0;

        /** @brief Execute the next instruction, stepping into function calls if needed, and return control to the debugger. */
        virtual void Step() = 0;

        /** @brief Execute the next instruction, stepping out of function calls if needed, and return control to the debugger. */
        virtual void StepOut() = 0;

        /** @brief Break the debugging process (stop the debuggee for debugging). */
        virtual void Break() = 0;

        /** @brief Stop the debugging process (exit debugging). */
        virtual void Stop() = 0;

        /** @brief Is the plugin currently debugging? */
        virtual bool IsRunning() const = 0;

        /** @brief Is the plugin stopped on breakpoint? */
        virtual bool IsStopped() const = 0;

        /** @brief Is the plugin processing something? */
        virtual bool IsBusy() const = 0;

        /** @brief Get the exit code of the last debug process. */
        virtual int GetExitCode() const = 0;

        // stack frame calls;
        virtual int GetStackFrameCount() const = 0;
        virtual cb::shared_ptr<const cbStackFrame> GetStackFrame(int index) const = 0;
        virtual void SwitchToFrame(int number) = 0;
        virtual int GetActiveStackFrame() const = 0;

        // breakpoints calls
        /** @brief Request to add a breakpoint.
          * @param file The file to add the breakpoint based on a file/line pair.
          * @param line The line number to put the breakpoint in @c file.
          * @return True if succeeded, false if not.
          */
        virtual cb::shared_ptr<cbBreakpoint> AddBreakpoint(const wxString& filename, int line) = 0;

        /** @brief Request to add a breakpoint based on a data expression.
          * @param dataExpression The data expression to add the breakpoint.
          * @return True if succeeded, false if not.
          */
        virtual cb::shared_ptr<cbBreakpoint> AddDataBreakpoint(const wxString& dataExpression) = 0;
        virtual int GetBreakpointsCount() const = 0;
        virtual cb::shared_ptr<cbBreakpoint> GetBreakpoint(int index) = 0;
        virtual cb::shared_ptr<const cbBreakpoint> GetBreakpoint(int index) const = 0;
        virtual void UpdateBreakpoint(cb::shared_ptr<cbBreakpoint> breakpoint) = 0;
        virtual void DeleteBreakpoint(cb::shared_ptr<cbBreakpoint> breakpoint) = 0;
        virtual void DeleteAllBreakpoints() = 0;
        virtual void ShiftBreakpoint(int index, int lines_to_shift) = 0;
        virtual void EnableBreakpoint(cb::shared_ptr<cbBreakpoint> breakpoint, bool enable) = 0;
        // threads
        virtual int GetThreadsCount() const = 0;
        virtual cb::shared_ptr<const cbThread> GetThread(int index) const = 0;
        virtual bool SwitchToThread(int thread_number) = 0;

        // watches

        /// Request to add a watch for a given symbol in your language.
        /// @param symbol Symbol or expression the debugger could understand and return a value for.
        /// @param update Pass true if you want the debugger to immediately read the value of the
        ///        symbol/expression, else it would be delayed until a call to UpdateWatch or
        ///        UpdateWatches is made or some stepping command finishes. Passing false might be
        ///        useful if you want to add multiple watches in one batch.
        virtual cb::shared_ptr<cbWatch> AddWatch(const wxString& symbol, bool update) = 0;
        /// Request to add a watch which would allow read/write access to a given memory range.
        /// @param address The start address of the range.
        /// @param size The size in bytes of the range.
        /// @param symbol The name of the watch shown in the UI.
        /// @param update Pass true if you want to make the debugger to immediately read the value
        ///        of the watch, else it would be delayed until UpdateWatch/UpdateWatches is called
        ///        or some stepping command finishes. Passing false is useful if you want to add
        ///        multiple watches in one batch.
        virtual cb::shared_ptr<cbWatch> AddMemoryRange(uint64_t address, uint64_t size, const wxString &symbol, bool update) = 0;
        virtual void DeleteWatch(cb::shared_ptr<cbWatch> watch) = 0;
        virtual bool HasWatch(cb::shared_ptr<cbWatch> watch) = 0;
        virtual void ShowWatchProperties(cb::shared_ptr<cbWatch> watch) = 0;
        virtual bool SetWatchValue(cb::shared_ptr<cbWatch> watch, const wxString& value) = 0;
        virtual void ExpandWatch(cb::shared_ptr<cbWatch> watch) = 0;
        virtual void CollapseWatch(cb::shared_ptr<cbWatch> watch) = 0;
        virtual void UpdateWatch(cb::shared_ptr<cbWatch> watch) = 0;

        /// Manually ask the debugger to read/update the values of the given list of watches.
        /// Depending on the debugger it might be more efficient than calling UpdateWatch multiple
        /// times. The default implementation does just that.
        /// @note The caller must make sure that all watches in the array are for this plugin.
        /// Passing watches for other plugins would have unexpected results. The plugins aren't
        /// required to check for this.
        virtual void UpdateWatches(const std::vector<cb::shared_ptr<cbWatch>> &watches);

        struct WatchesDisabledMenuItems
        {
            enum
            {
                Empty        = 0,
                Rename       = 1 << 0,
                Properties   = 1 << 1,
                Delete       = 1 << 2,
                DeleteAll    = 1 << 3,
                AddDataBreak = 1 << 4,
                ExamineMemory = 1 << 5
            };
        };

        /**
          * @param[out] disabledMenus A combination of WatchesDisabledMenuItems, which controls which of the default menu items are disabled
          */
        virtual void OnWatchesContextMenu(wxMenu &menu, const cbWatch &watch, wxObject *property, int &disabledMenus)
        { wxUnusedVar(menu); wxUnusedVar(watch); wxUnusedVar(property); wxUnusedVar(disabledMenus); };

        virtual void SendCommand(const wxString& cmd, bool debugLog) = 0;

        virtual void AttachToProcess(const wxString& pid) = 0;
        virtual void DetachFromProcess() = 0;
        virtual bool IsAttachedToProcess() const = 0;

        virtual void GetCurrentPosition(wxString& filename, int &line) = 0;


        virtual void OnValueTooltip(const wxString& token, const wxRect &evalRect);
        virtual bool ShowValueTooltip(int style);
    private:
        void RegisterValueTooltip();
        void ProcessValueTooltip(CodeBlocksEvent& event);
        void CancelValueTooltip(CodeBlocksEvent& event);

    protected:
        enum StartType
        {
            StartTypeUnknown = 0,
            StartTypeRun,
            StartTypeStepInto
        };
    protected:
        virtual void ConvertDirectory(wxString& str, wxString base = _T(""), bool relative = true) = 0;
        virtual cbProject* GetProject() = 0;
        virtual void ResetProject() = 0;
        virtual void CleanupWhenProjectClosed(cbProject *project) = 0;

        /** @brief Called when the compilation has finished. The compilation is started when EnsureBuildUpToDate is called.
          * @param compilerFailed the compilation failed for some reason.
          * @param startType it is the same value given to the Debug method, when the debugger session was started.
          * @return True if debug session is start, false if there are any errors or the users canceled the session.
        */
        virtual bool CompilerFinished(bool compilerFailed, StartType startType)
        { wxUnusedVar(compilerFailed); wxUnusedVar(startType); return false; }
    public:
        enum DebugWindows
        {
            Backtrace,
            CPURegisters,
            Disassembly,
            ExamineMemory,
            MemoryRange,
            Threads,
            Watches
        };

        virtual void RequestUpdate(DebugWindows window) = 0;

    public:
        virtual wxString GetEditorWordAtCaret(const wxPoint *mousePosition = NULL);
        void ClearActiveMarkFromAllEditors();

        enum SyncEditorResult
        {
            SyncOk = 0,
            SyncFileNotFound,
            SyncFileUnknown
        };

        SyncEditorResult SyncEditor(const wxString& filename, int line, bool setMarker = true);

        void BringCBToFront();


        void ShowLog(bool clear);
        void Log(const wxString& msg, Logger::level level = Logger::info);
        void DebugLog(const wxString& msg, Logger::level level = Logger::info);
        bool HasDebugLog() const;
        void ClearLog();

        // Called only by DebuggerManager, when registering plugin or changing settings
        void SetupLog(int normalIndex);

        wxString GetGUIName() const { return m_guiName; }
        wxString GetSettingsName() const { return m_settingsName; }

    protected:
        void SwitchToDebuggingLayout();
        void SwitchToPreviousLayout();

        bool GetDebuggee(wxString& pathToDebuggee, wxString& workingDirectory, ProjectBuildTarget* target);
        bool EnsureBuildUpToDate(StartType startType);
        bool WaitingCompilerToFinish() const { return m_WaitingCompilerToFinish; }

        int RunNixConsole(wxString& consoleTty);
        void MarkAsStopped();

    private:
        void OnEditorOpened(CodeBlocksEvent& event);
        void OnProjectActivated(CodeBlocksEvent& event);
        void OnProjectClosed(CodeBlocksEvent& event);
        void OnCompilerFinished(CodeBlocksEvent& event);
    private:
        wxString m_PreviousLayout;
        cbCompilerPlugin* m_pCompiler;
        bool m_WaitingCompilerToFinish;

        StartType m_StartType;

        int m_ActiveConfig;

        int m_LogPageIndex;
        bool m_lastLineWasNormal;
        wxString m_guiName, m_settingsName;
};

/** @brief Base class for tool plugins
  *
  * This plugin is automatically managed by Code::Blocks, so the inherited
  * functions to build menus/toolbars are hidden.
  *
  * Tool plugins are automatically added under the "Plugins" menu.
  */
class PLUGIN_EXPORT cbToolPlugin : public cbPlugin
{
    public:
        cbToolPlugin();

        /** @brief Execute the plugin.
          *
          * This is the only function needed by a cbToolPlugin.
          * This will be called when the user selects the plugin from the "Plugins"
          * menu.
          */
        virtual int Execute() = 0;
    private:
        // "Hide" some virtual members, that are not needed in cbToolPlugin
        void BuildMenu(cb_unused wxMenuBar* menuBar) override{}
        void RemoveMenu(cb_unused wxMenuBar* menuBar){}
        void BuildModuleMenu(cb_unused const ModuleType type, cb_unused wxMenu* menu, cb_unused const FileTreeData* data = nullptr) override{}
        bool BuildToolBar(cb_unused wxToolBar* toolBar) override{ return false; }
        void RemoveToolBar(cb_unused wxToolBar* toolBar){}
};

/** @brief Base class for mime plugins
  *
  * Mime plugins are called by Code::Blocks to operate on files that Code::Blocks
  * wouldn't know how to handle on itself.
  */
class PLUGIN_EXPORT cbMimePlugin : public cbPlugin
{
    public:
        cbMimePlugin();

        /** @brief Can a file be handled by this plugin?
          *
          * @param filename The file in question.
          * @return The plugin should return true if it can handle this file,
          * false if not.
          */
        virtual bool CanHandleFile(const wxString& filename) const = 0;

        /** @brief Open the file.
          *
          * @param filename The file to open.
          * @return The plugin should return zero on success, other value on error.
          */
        virtual int OpenFile(const wxString& filename) = 0;

        /** @brief Is this a default handler?
          *
          * This is a flag notifying the main app that this plugin can handle
          * every file passed to it. Usually you 'll want to return false in
          * this function, because you usually create specialized handler
          * plugins (for specific MIME types)...
          *
          * @return True if this plugin can handle every possible MIME type,
          * false if not.
          */
        virtual bool HandlesEverything() const = 0;
    private:
        // "Hide" some virtual members, that are not needed in cbMimePlugin
        void BuildMenu(cb_unused wxMenuBar* menuBar) override{}
        void RemoveMenu(cb_unused wxMenuBar* menuBar){}
        void BuildModuleMenu(cb_unused const ModuleType type, cb_unused wxMenu* menu, cb_unused const FileTreeData* data = nullptr) override{}
        bool BuildToolBar(cb_unused wxToolBar* toolBar) override{ return false; }
        void RemoveToolBar(cb_unused wxToolBar* toolBar){}
};

class wxHtmlLinkEvent;

/** @brief Base class for code-completion plugins
  *
  * The main operations of a code-completion plugin are executed by CCManager
  * at the appropriate times. Smaller CC plugins *should* not have need to
  * register very many (if any) events/editor hooks.
  */
class PLUGIN_EXPORT cbCodeCompletionPlugin : public cbPlugin
{
    public:
        cbCodeCompletionPlugin();

        /** Level of functionality a CC plugin is able to provide. */
        enum CCProviderStatus
        {
            ccpsInactive, //!< CC plugin provides no functionality.
            ccpsActive,   //!< CC plugin provides specialized functionality.
            ccpsUniversal //!< CC plugin provides generic functionality.
        };

        /** Structure representing a generic token, passed between CC plugins and CCManager. */
        struct CCToken
        {
            /** @brief Convenience constructor.
              *
              * Represents a generic token, passed between CC plugins and CCManager.
              *
              * @param _id Internal identifier for a CC plugin to reference the token in its data structure.
              * @param dispNm The string CCManager will use to display this token.
              * @param categ The category corresponding to the index of the registered image (during autocomplete).
              *              Negative values are reserved for CCManager.
              */
            CCToken(int _id, const wxString& dispNm, int categ = -1) :
                id(_id), category(categ), weight(5), displayName(dispNm), name(dispNm) {}

            /** @brief Construct a fully specified CCToken.
              *
              * Represents a generic token, passed between CC plugins and CCManager.
              *
              * @param _id Internal identifier for a CC plugin to reference the token in its data structure.
              * @param dispNm The verbose string CCManager will use to display this token.
              * @param nm Minimal name of the token that CCManager may choose to display in restricted circumstances.
              * @param _weight Lower numbers are placed earlier in listing, 5 is default; try to keep 0-10.
              * @param categ The category corresponding to the index of the registered image (during autocomplete).
              *              Negative values are reserved for CCManager.
              */
            CCToken(int _id, const wxString& dispNm, const wxString& nm, int _weight, int categ = -1) :
                id(_id), category(categ), weight(_weight), displayName(dispNm), name(nm) {}

            int id;               //!< CCManager will pass this back unmodified. Use it as an internal identifier for the token.
            int category;         //!< The category corresponding to the index of the registered image (during autocomplete).
            int weight;           //!< Lower numbers are placed earlier in listing, 5 is default; try to keep 0-10.
            wxString displayName; //!< Verbose string representing the token.
            wxString name;        //!< Minimal name of the token that CCManager may choose to display in restricted circumstances.
        };

        /** Structure representing an individual calltip with an optional highlighted range */
        struct CCCallTip
        {
            /** @brief Convenience constructor.
              *
              * Represents an individual calltip to be processed and displayed by CCManager.
              *
              * @param tp The content of the calltip.
              */
            CCCallTip(const wxString& tp) :
                hlStart(-1), hlEnd(-1), tip(tp) {}

            /** @brief Construct a calltip, specifying a highlighted range
              *
              * Represents an individual calltip, containing a highlighted range (generally the
              * active parameter), to be processed and displayed by CCManager.
              *
              * @param tp The content of the calltip.
              * @param highlightStart The start index of the desired highlighted range.
              * @param highlightEndThe end index of the desired highlighted range.
              */
            CCCallTip(const wxString& tp, int highlightStart, int highlightEnd) :
                hlStart(highlightStart), hlEnd(highlightEnd), tip(tp) {}

            int hlStart;  //!< The start index of the desired highlighted range.
            int hlEnd;    //!< The end index of the desired highlighted range.
            wxString tip; //!< The content of the calltip.
        };

        /** @brief Does this plugin handle code completion for the editor <tt>ed</tt>?
          *
          * The plugin should check the lexer, the <tt>HighlightLanguage</tt>, the file extension,
          * or some combination of these. Do @em not call @c CCManager::GetProviderFor()
          * from this function.
          *
          * @param ed The editor being checked.
          * @return The level of functionality this plugin is able to supply.
          */
        virtual CCProviderStatus GetProviderStatusFor(cbEditor* ed) = 0;

        /** @brief Supply content for the autocompletion list.
          *
          * CCManager takes care of calling this during most relevant situations. If the
          * autocompletion mechanism is required at a time that CCManager does not initiate, call
          * @code
          * CodeBlocksEvent evt(cbEVT_COMPLETE_CODE);
          * Manager::Get()->ProcessEvent(evt);
          * @endcode
          * In this case, the parameter isAuto is passed as false value.
          *
          * Here is an example
          * @code
          * #include <math.h>
          * int main()
          * {
          *     float i = cos|------auto completion here
          *               ^  ^
          * }
          * @endcode
          * This is the case the user has just enter the chars "cos", now to get a suggestion list.
          * The first '^' is the position of tknStart, and the second '^' is the position of tknEnd
          * In this case, the cc plugin would supply a CCToken vectors, which could contains "cos",
          * "cosh" and "cosh"...
          * In some special cases, the tknStart tknEnd may point to the same position, such as
          * @code
          * struct AAA { int m_aaa1; };
          * int main()
          * {
          *     AAA obj;
          *     obj.|------auto completion here
          *         ^
          * }
          * @endcode
          * Here, '^' are the positions of both tknStart and tknEnd.
          *
          * @param isAuto Passed as @c true if autocompletion was launched by typing an 'interesting'
          *               character such as '<tt>&gt;</tt>' (for '<tt>-&gt;</tt>'). It is the plugin's job
          *               to filter out incorrect calls of this.
          * @param ed The context of this codecompletion call.
          * @param[in,out] tknStart The assumed beginning of the token to be autocompleted. Change this variable
          *                         if the plugin calculates a different starting location.
          * @param[in,out] tknEnd The current position/end of the known part of the token to be completed. The
          *                       plugin is allowed to change this (but it is not recommended).
          * @return Completable tokens, or empty vector to cancel autocompletion.
          */
        virtual std::vector<CCToken> GetAutocompList(bool isAuto, cbEditor* ed, int& tknStart, int& tknEnd) = 0;

        /** @brief Supply html formatted documentation for the passed token.
          *
          * Refer to http://docs.wxwidgets.org/stable/overview_html.html#overview_html_supptags for
          * the available formatting. When selecting colours, prefer use of the ones CCManager has
          * registered with ColourManager, which are (TODO: register colours). Returning an empty
          * string will cancel the documentation popup.
          *
          * @param token The token to document.
          * @return Either an html document or an empty string (if no documentation available).
          */
        virtual wxString GetDocumentation(const CCToken& token) = 0;

        /** @brief Supply content for the calltip at the specified location.
          *
          * The output parameter @c argsPos is required to be set to the same (but unique) position
          * for each unique calltip. This position is the location corresponding to the beginning of
          * the argument list:
          * @code
          * int endOfWord = stc->WordEndPosition(pos, true);
          *                                     ^
          * @endcode
          * Each returned CCCallTip is allowed to have embedded '\\n' line breaks.
          *
          * @param pos The location in the editor that the calltip is requested for.
          * @param style The scintilla style of the cbStyledTextCtrl at the given location. (TODO: This
          *              is unusual, remove it?)
          * @param ed The context of this calltip request.
          * @param[out] argsPos The location in the editor of the beginning of the argument list. @em Required.
          * @return Each entry in this vector is guaranteed either a new line or a separate page in the calltip.
          *         CCManager will decide if lines should be further split (for formatting to fit the monitor).
          */
        virtual std::vector<CCCallTip> GetCallTips(int pos, int style, cbEditor* ed, int& argsPos) = 0;

        /** @brief Supply the definition of the token at the specified location.
          *
          * The token(s) returned by this function are used to display tooltips.
          *
          * @param pos The location being queried.
          * @param ed The context of the request.
          * @param[out] allowCallTip Allow CCManager to consider displaying a calltip if the results from this
          *                          function are unsuitable/empty. True by default.
          * @return A list of the token(s) that match the specified location, an empty vector if none.
          */
        virtual std::vector<CCToken> GetTokenAt(int pos, cbEditor* ed, bool& allowCallTip) = 0;

        /** @brief Callback to handle a click on a link in the documentation popup.
         *
         * Handle a link command by, for example, showing the definition of a member function, or opening
         * an editor to the location of the declaration.
         *
         * @param event The generated event (it is the plugin's responsibility to Skip(), if desired).
         * @param[out] dismissPopup If set to true, the popup will be hidden.
         * @return If non-empty, the popup's content will be set to this html formatted string.
         */
        virtual wxString OnDocumentationLink(wxHtmlLinkEvent& event, bool& dismissPopup) = 0;

        /** @brief Callback for inserting the selected autocomplete entry into the editor.
          *
          * The default implementation executes (wx)Scintilla's insert. Override and call
          * @c ed->GetControl()->AutoCompCancel() for different @c wxEVT_SCI_AUTOCOMP_SELECTION behaviour.
          *
          * @param token The CCToken corresponding to the selected entry.
          * @param ed The editor to operate in.
          */
        virtual void DoAutocomplete(const CCToken& token, cbEditor* ed);

        /** @brief Callback for inserting the selected autocomplete entry into the editor.
          *
          * This function is only called if CCManager fails to retrieve the CCToken associated with the
          * selection (which should never happen). The default implementation creates a CCToken and passes
          * it to <tt>DoAutocomplete(const CCToken&, cbEditor*)</tt><br>
          * Override for different behaviour.
          *
          * @param token A string corresponding to the selected entry.
          * @param ed The editor to operate in.
          */
        virtual void DoAutocomplete(const wxString& token, cbEditor* ed);

        virtual bool DoShowDiagnostics( cbEditor* ed, int line) {return false;}


    protected:
        /** @brief Has this plugin been selected to provide content for the editor.
          *
          * Convenience function; asks CCManager if this plugin is granted jurisdiction over the editor.
          *
          * @param ed The editor to check.
          * @return Is provider for the editor.
          */
        bool IsProviderFor(cbEditor* ed);
};

/** @brief Base class for wizard plugins
  *
  * Wizard plugins are called by Code::Blocks when the user selects
  * "File->New...".
  *
  * A plugin of this type can support more than one wizard. Additionally,
  * each wizard can support new workspaces, new projects, new targets or new files.
  * The @c index used as a parameter to most of the functions, denotes 0-based index
  * of the project wizard to run.
  */
class PLUGIN_EXPORT cbWizardPlugin : public cbPlugin
{
    public:
        cbWizardPlugin();

        /** @return the number of template wizards this plugin contains */
        virtual int GetCount() const = 0;

        /** @param index the wizard index.
          * @return the output type of the specified wizard at @c index */
        virtual TemplateOutputType GetOutputType(int index) const = 0;

        /** @param index the wizard index.
          * @return the template's title */
        virtual wxString GetTitle(int index) const = 0;

        /** @param index the wizard index.
          * @return the template's description */
        virtual wxString GetDescription(int index) const = 0;

        /** @param index the wizard index.
          * @return the template's category (GUI, Console, etc; free-form text). Try to adhere to standard category names... */
        virtual wxString GetCategory(int index) const = 0;

        /** @param index the wizard index.
          * @return the template's bitmap */
        virtual const wxBitmap& GetBitmap(int index) const = 0;

        /** @param index the wizard index.
          * @return this wizard's script filename (if this wizard is scripted). */
        virtual wxString GetScriptFilename(int index) const = 0;

        /** When this is called, the wizard must get to work ;).
          * @param index the wizard index.
          * @param createdFilename if provided, on return it should contain the main filename
          *                         this wizard created. If the user created a project, that
          *                         would be the project's filename.
          *                         If the wizard created a build target, that would be an empty string.
          *                         If the wizard created a file, that would be the file's name.
          * @return a pointer to the generated cbProject or ProjectBuildTarget. NULL for everything else (failure too).
          * You should dynamic-cast this to the correct type based on GetOutputType() 's value. */
        virtual CompileTargetBase* Launch(int index, wxString* createdFilename = nullptr) = 0; // do your work ;)
    private:
        // "Hide" some virtual members, that are not needed in cbCreateWizardPlugin
        void BuildMenu(cb_unused wxMenuBar* menuBar) override{}
        void RemoveMenu(cb_unused wxMenuBar* menuBar){}
        void BuildModuleMenu(cb_unused const ModuleType type, cb_unused wxMenu* menu, cb_unused const FileTreeData* data = nullptr) override{}
        bool BuildToolBar(cb_unused wxToolBar* toolBar) override{ return false; }
        void RemoveToolBar(cb_unused wxToolBar* toolBar){}
};

/** @brief Base class for SmartIndent plugins
  *
  * SmartIndent plugins provide the smart indenting for different languages.
  * These plugins don't eat processing time after startup when they are not active.
  * The hook gets installed during OnAttach.
  */
class cbStyledTextCtrl;
class PLUGIN_EXPORT cbSmartIndentPlugin : public cbPlugin
{
    public:
        cbSmartIndentPlugin();
    private:
        // "Hide" some virtual members, that are not needed in cbSmartIndentPlugin
        void BuildMenu(cb_unused wxMenuBar* menuBar) override{}
        void RemoveMenu(cb_unused wxMenuBar* menuBar){}
        void BuildModuleMenu(cb_unused const ModuleType type, cb_unused wxMenu* menu, cb_unused const FileTreeData* data = nullptr) override{}
        bool BuildToolBar(cb_unused wxToolBar* toolBar) override{ return false; }
        void RemoveToolBar(cb_unused wxToolBar* toolBar){}
    protected:
        void OnAttach() override;
        void OnRelease(bool appShutDown) override;

    public:
        /** When this is called, the smartIndent mechanism must get to work ;).
          *
          * Please check if this is the right smartIndent mechanism first:
          * Don't indent for languages you don't know.
          */
        virtual void OnEditorHook(cbEditor* editor, wxScintillaEvent& event) const = 0;

        /** This is called after a code completion operation finishes.
          *
          * Use it as an opportunity to tidy up CC's formating.
          * Don't indent for languages you don't know.
          */
        virtual void OnCCDone(cb_unused cbEditor* ed){}

    protected:
        /** (reverse) search for the last word which is not comment **/
        wxString GetLastNonCommentWord(cbEditor* ed, int position = -1, unsigned int NumberOfWords = 1 ) const;
        /** (reverse) search for the last characters, which are not whitespace and not comment **/
        wxString GetLastNonWhitespaceChars(cbEditor* ed, int position = -1, unsigned int NumberOfChars = 1) const;

        /** forward search to the next character which is not a whitespace **/
        wxChar GetLastNonWhitespaceChar(cbEditor* ed, int position = -1) const;
        wxChar GetNextNonWhitespaceCharOnLine(cbStyledTextCtrl* stc, int position = -1, int *pos = nullptr) const;

        int FindBlockStart(cbStyledTextCtrl* stc, int position, wxChar blockStart, wxChar blockEnd, bool skipNested = true) const;
        int FindBlockStart(cbStyledTextCtrl* stc, int position, wxString blockStart, wxString blockEnd, bool CaseSensitive = true) const;

        void Indent(cbStyledTextCtrl* stc, wxString& indent)const;
        bool Indent(cbStyledTextCtrl* stc, wxString& indent, int posInLine)const;

        /** Get the first brace in the line according to the line style */
        int GetFirstBraceInLine(cbStyledTextCtrl* stc, int string_style)const;

        /** Get the last non-whitespace character from position in line */
        wxChar GetNextNonWhitespaceCharOfLine(cbStyledTextCtrl* stc, int position = -1, int *pos = nullptr)const;
        bool AutoIndentEnabled()const;
        bool SmartIndentEnabled()const;
        bool BraceSmartIndentEnabled()const;
        bool BraceCompletionEnabled()const;
        bool SelectionBraceCompletionEnabled()const;
        void OnCCDoneEvent(CodeBlocksEvent& event);
    private:
        int m_FunctorId;
};

/** @brief Plugin registration object.
  *
  * Use this class to register your new plugin with Code::Blocks.
  * All you have to do is instantiate a PluginRegistrant object.
  * @par
  * Example code to use in one of your plugin's source files (supposedly called "MyPlugin"):
  * @code
  * namespace
  * {
  *     PluginRegistrant<MyPlugin> registration("MyPlugin");
  * }
  * @endcode
  */
template<class T> class PluginRegistrant
{
    public:
        /// @param name The plugin's name.
        PluginRegistrant(const wxString& name)
        {
            Manager::Get()->GetPluginManager()->RegisterPlugin(name, // plugin's name
                                                                &CreatePlugin, // creation
                                                                &FreePlugin, // destruction
                                                                &SDKVersion); // SDK version
        }

        static cbPlugin* CreatePlugin()
        {
            return new T;
        }

        static void FreePlugin(cbPlugin* plugin)
        {
            delete plugin;
        }

        static void SDKVersion(int* major, int* minor, int* release)
        {
            if (major) *major = PLUGIN_SDK_VERSION_MAJOR;
            if (minor) *minor = PLUGIN_SDK_VERSION_MINOR;
            if (release) *release = PLUGIN_SDK_VERSION_RELEASE;
        }
};

#endif // CBPLUGIN_H
