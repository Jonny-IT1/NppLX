// NppLX - tabbed text editor in the style of Notepad++ for Linux
// C++17 / Qt6 Widgets / QScintilla (Scintilla editing component)
// License: GPL-3.0-or-later. Independent reimplementation; not a
// part of Notepad++, and not endorsed by its maintainers.

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QColor>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QPalette>
#include <QPair>
#include <QPrintDialog>
#include <QPrinter>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QStringDecoder>
#include <QStringList>
#include <QStyle>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>
#include <QScrollBar>
#include <QElapsedTimer>
#include <QHash>
#include <QProcess>
#include <QTemporaryDir>
#include <QUrl>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QVector>
#include <Qsci/qsciscintilla.h>

#include <algorithm>
#include <functional>
#include <Qsci/qsciscintillabase.h>
#include <Qsci/qscilexer.h>
#include <Qsci/qscilexerbash.h>
#include <Qsci/qscilexercpp.h>
#include <Qsci/qscilexercsharp.h>
#include <Qsci/qscilexercss.h>
#include <Qsci/qscilexerdiff.h>
#include <Qsci/qscilexerhtml.h>
#if __has_include(<Qsci/qscilexerini.h>)
#define NPPLX_HAS_INI 1
#include <Qsci/qscilexerini.h>
#else
#define NPPLX_HAS_INI 0
#include <Qsci/qscilexerproperties.h>
#endif
#include <Qsci/qscilexerjava.h>
#include <Qsci/qscilexerjavascript.h>
#include <Qsci/qscilexerlua.h>
#include <Qsci/qscilexermakefile.h>
#include <Qsci/qscilexerpascal.h>
#include <Qsci/qscilexerpython.h>
#include <Qsci/qscilexerruby.h>
#include <Qsci/qscilexersql.h>
#include <Qsci/qscilexerxml.h>

#if __has_include(<Qsci/qscilexerjson.h>)
#define NPPLX_HAS_JSON 1
#include <Qsci/qscilexerjson.h>
#else
#define NPPLX_HAS_JSON 0
#endif
#if __has_include(<Qsci/qscilexermarkdown.h>)
#define NPPLX_HAS_MARKDOWN 1
#include <Qsci/qscilexermarkdown.h>
#else
#define NPPLX_HAS_MARKDOWN 0
#endif
#if __has_include(<Qsci/qscilexeryaml.h>)
#define NPPLX_HAS_YAML 1
#include <Qsci/qscilexeryaml.h>
#else
#define NPPLX_HAS_YAML 0
#endif
#if __has_include(<Qsci/qscilexercmake.h>)
#define NPPLX_HAS_CMAKE 1
#include <Qsci/qscilexercmake.h>
#else
#define NPPLX_HAS_CMAKE 0
#endif

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class MainWindow;

namespace {

const int MaxRecent = 10;
const int DefaultFontSize = 10;
const int MinFontSize = 6;
const int MaxFontSize = 40;

// Color roles for syntax colors: role -> (dark, light)
QColor roleColor(const std::string &role, bool dark)
{
    static const std::unordered_map<std::string,
                                    std::pair<const char *, const char *>> M = {
        {"comment",   {"#928374", "#7c6f64"}},
        {"string",    {"#b8bb26", "#797403"}},
        {"number",    {"#d3869b", "#8f3f71"}},
        {"keyword",   {"#fb4934", "#9d0006"}},
        {"type",      {"#fabd2f", "#b57614"}},
        {"operator",  {"#ebdbb2", "#3c3836"}},
        {"tag",       {"#83a598", "#076678"}},
        {"preproc",   {"#fe8019", "#af3a03"}},
        {"decorator", {"#8ec07c", "#427b58"}},
        {"default",   {"#ebdbb2", "#3c3836"}},
    };
    const auto it = M.find(role);
    if (it == M.end())
        return QColor(dark ? "#ebdbb2" : "#3c3836");
    return QColor(dark ? it->second.first : it->second.second);
}

// Syntax accent roles via stable Scintilla style numbers (style numbers are
// part of the Scintilla ABI and never change, unlike enum identifiers); the
// roles resolve per theme in roleColor().
const std::unordered_map<std::string,
                        std::vector<std::pair<int, const char *>>> DarkStyles = {
    {"Python", {{1, "comment"}, {2, "number"}, {3, "string"}, {4, "string"},
                {5, "string"}, {6, "string"}, {7, "type"}, {8, "type"},
                {9, "operator"}, {10, "default"}, {12, "comment"},
                {13, "string"}, {15, "decorator"}}},
    {"C++",     {{1, "comment"}, {2, "comment"}, {3, "comment"},
                {4, "number"}, {5, "keyword"}, {6, "string"}, {7, "string"},
                {9, "preproc"}, {10, "operator"}, {11, "default"},
                {13, "string"}, {14, "string"}, {17, "comment"},
                {18, "comment"}}},
    {"C#",      {{1, "comment"}, {2, "comment"}, {3, "comment"},
                {4, "number"}, {5, "keyword"}, {6, "string"}, {7, "string"},
                {9, "preproc"}, {10, "operator"}, {11, "default"},
                {13, "string"}, {14, "string"}}},
    {"Java",    {{1, "comment"}, {2, "comment"}, {3, "comment"},
                {4, "number"}, {5, "keyword"}, {6, "string"}, {7, "string"},
                {9, "preproc"}, {10, "operator"}, {11, "default"},
                {13, "string"}, {14, "string"}}},
    {"JavaScript", {{1, "comment"}, {2, "comment"}, {3, "comment"},
                    {4, "number"}, {5, "keyword"}, {6, "string"},
                    {7, "string"}, {9, "preproc"}, {10, "operator"},
                    {11, "default"}, {13, "string"}, {14, "string"}}},
    {"HTML",    {{1, "tag"}, {2, "tag"}, {5, "string"}, {6, "string"},
                {9, "comment"}}},
    {"XML",     {{1, "tag"}, {5, "string"}, {6, "string"}, {9, "comment"}}},
    {"CSS",     {{1, "comment"}, {2, "tag"}}},
    {"Bash",    {{2, "comment"}, {3, "number"}, {4, "keyword"},
                {5, "string"}, {6, "string"}, {7, "operator"},
                {9, "decorator"}, {10, "default"}}},
    {"SQL",     {{1, "comment"}, {2, "comment"}}},
#if NPPLX_HAS_INI
    {"INI",     {{1, "comment"}, {2, "type"}}},
#else
    {"INI",     {{1, "comment"}}},
#endif
    {"Makefile", {{1, "comment"}}},
    {"Lua",     {{1, "comment"}, {2, "comment"}}},
    {"Ruby",    {{1, "comment"}, {2, "number"}, {3, "string"},
                {4, "keyword"}}},
    {"Pascal",  {{1, "comment"}}},
    {"Diff",    {{1, "comment"}}},
#if NPPLX_HAS_JSON
    {"JSON",    {{2, "number"}, {3, "string"}, {4, "keyword"}}},
#endif
#if NPPLX_HAS_YAML
    {"YAML",    {{1, "comment"}}},
#endif
#if NPPLX_HAS_CMAKE
    {"CMake",   {{1, "comment"}}},
#endif
};

QsciLexer *makeLexer(const QString &name, QObject *parent)
{
    if (name == QStringLiteral("Python"))      return new QsciLexerPython(parent);
    if (name == QStringLiteral("C++"))         return new QsciLexerCPP(parent);
    if (name == QStringLiteral("C#"))          return new QsciLexerCSharp(parent);
    if (name == QStringLiteral("Java"))        return new QsciLexerJava(parent);
    if (name == QStringLiteral("JavaScript"))  return new QsciLexerJavaScript(parent);
    if (name == QStringLiteral("HTML"))       return new QsciLexerHTML(parent);
    if (name == QStringLiteral("XML"))        return new QsciLexerXML(parent);
    if (name == QStringLiteral("CSS"))        return new QsciLexerCSS(parent);
    if (name == QStringLiteral("Bash"))      return new QsciLexerBash(parent);
    if (name == QStringLiteral("SQL"))        return new QsciLexerSQL(parent);
#if NPPLX_HAS_INI
    if (name == QStringLiteral("INI"))        return new QsciLexerIni(parent);
#else
    if (name == QStringLiteral("INI"))        return new QsciLexerProperties(parent);
#endif
    if (name == QStringLiteral("Makefile"))   return new QsciLexerMakefile(parent);
    if (name == QStringLiteral("Lua"))        return new QsciLexerLua(parent);
    if (name == QStringLiteral("Ruby"))       return new QsciLexerRuby(parent);
    if (name == QStringLiteral("Pascal"))    return new QsciLexerPascal(parent);
    if (name == QStringLiteral("Diff"))       return new QsciLexerDiff(parent);
#if NPPLX_HAS_JSON
    if (name == QStringLiteral("JSON"))       return new QsciLexerJSON(parent);
#endif
#if NPPLX_HAS_MARKDOWN
    if (name == QStringLiteral("Markdown"))   return new QsciLexerMarkdown(parent);
#endif
#if NPPLX_HAS_YAML
    if (name == QStringLiteral("YAML"))       return new QsciLexerYAML(parent);
#endif
#if NPPLX_HAS_CMAKE
    if (name == QStringLiteral("CMake"))     return new QsciLexerCMake(parent);
#endif
    return nullptr;
}

QString langForPath(const QString &path)
{
    static const std::unordered_map<std::string, QString> ExtMap = {
        {"py", QStringLiteral("Python")}, {"pyw", QStringLiteral("Python")},
        {"c", QStringLiteral("C++")}, {"h", QStringLiteral("C++")},
        {"cpp", QStringLiteral("C++")}, {"cc", QStringLiteral("C++")},
        {"cxx", QStringLiteral("C++")}, {"hpp", QStringLiteral("C++")},
        {"hh", QStringLiteral("C++")}, {"inl", QStringLiteral("C++")},
        {"cs", QStringLiteral("C#")},
        {"java", QStringLiteral("Java")},
        {"js", QStringLiteral("JavaScript")}, {"mjs", QStringLiteral("JavaScript")},
        {"jsx", QStringLiteral("JavaScript")}, {"ts", QStringLiteral("JavaScript")},
        {"tsx", QStringLiteral("JavaScript")},
        {"html", QStringLiteral("HTML")}, {"htm", QStringLiteral("HTML")},
        {"php", QStringLiteral("HTML")}, {"vue", QStringLiteral("HTML")},
        {"xml", QStringLiteral("XML")}, {"svg", QStringLiteral("XML")},
        {"xsl", QStringLiteral("XML")}, {"ui", QStringLiteral("XML")},
        {"glade", QStringLiteral("XML")}, {"kml", QStringLiteral("XML")},
        {"qml", QStringLiteral("XML")},
        {"css", QStringLiteral("CSS")}, {"scss", QStringLiteral("CSS")},
        {"less", QStringLiteral("CSS")},
        {"json", QStringLiteral("JSON")}, {"jsonc", QStringLiteral("JSON")},
        {"ipynb", QStringLiteral("JSON")},
        {"md", QStringLiteral("Markdown")}, {"markdown", QStringLiteral("Markdown")},
        {"yaml", QStringLiteral("YAML")}, {"yml", QStringLiteral("YAML")},
        {"ini", QStringLiteral("INI")}, {"cfg", QStringLiteral("INI")},
        {"conf", QStringLiteral("INI")}, {"desktop", QStringLiteral("INI")},
        {"service", QStringLiteral("INI")}, {"properties", QStringLiteral("INI")},
        {"toml", QStringLiteral("INI")},
        {"sh", QStringLiteral("Bash")}, {"bash", QStringLiteral("Bash")},
        {"zsh", QStringLiteral("Bash")}, {"ksh", QStringLiteral("Bash")},
        {"sql", QStringLiteral("SQL")},
        {"mak", QStringLiteral("Makefile")}, {"mk", QStringLiteral("Makefile")},
        {"cmake", QStringLiteral("CMake")},
        {"lua", QStringLiteral("Lua")},
        {"rb", QStringLiteral("Ruby")}, {"ruby", QStringLiteral("Ruby")},
        {"pas", QStringLiteral("Pascal")}, {"pp", QStringLiteral("Pascal")},
        {"dpr", QStringLiteral("Pascal")},
        {"diff", QStringLiteral("Diff")}, {"patch", QStringLiteral("Diff")},
    };
    const QString lowered = QFileInfo(path).fileName().toLower();
    if (lowered == QLatin1String("makefile")
            || lowered.startsWith(QLatin1String("makefile.")))
        return QStringLiteral("Makefile");
    const std::string ext = QFileInfo(path).suffix().toLower().toStdString();
    const auto it = ExtMap.find(ext);
    return it == ExtMap.end() ? QString() : it->second;
}

QString resolveMonoFamily()
{
    const QStringList candidates = {
        QStringLiteral("DejaVu Sans Mono"),
        QStringLiteral("JetBrains Mono"),
        QStringLiteral("Fira Code"),
        QStringLiteral("Liberation Mono"),
        QStringLiteral("Noto Sans Mono"),
        QStringLiteral("Ubuntu Mono"),
        QStringLiteral("Consolas"),
        QStringLiteral("Menlo"),
        QStringLiteral("Courier New"),
    };
    const QStringList families = QFontDatabase::families();
    for (const QString &family : candidates)
        if (families.contains(family))
            return family;
    return QStringLiteral("Monospace");
}

} // namespace

// ---------------------------------------------------------------------------
// Editor (counterpart of ScintillaEditView in the original)
// ---------------------------------------------------------------------------

class Editor : public QsciScintilla
{
public:
    explicit Editor(QWidget *parent = nullptr)
        : QsciScintilla(parent)
    {
        setUtf8(true);
        setCaretLineVisible(true);
        setAutoIndent(true);
        setTabIndents(true);
        setBackspaceUnindents(true);
        setIndentationsUseTabs(false);
        setTabWidth(4);
        setEdgeColumn(100);
        setEdgeMode(QsciScintilla::EdgeLine);
        setBraceMatching(QsciScintilla::SloppyBraceMatch);
        setMarginType(0, QsciScintilla::NumberMargin);
        setMarginLineNumbers(0, true);
        setMarginType(1, QsciScintilla::SymbolMargin);
        setMarginSensitivity(1, true);
        setMarginType(2, QsciScintilla::SymbolMargin);   // bookmark margin
        setMarginSensitivity(2, true);
        bmMarker = markerDefine(QsciScintilla::Bookmark);
        setFolding(QsciScintilla::BoxedTreeFoldStyle);
        setAutoCompletionSource(QsciScintilla::AcsDocument);
        setAutoCompletionThreshold(2);
    }

    QString filepath;      // empty = untitled
    QString encoding = QStringLiteral("utf-8");
    QString lang;          // empty = normal text
    int bmMarker = -1;     // marker number of the bookmarks on margin 2

    QString baseName() const
    {
        return filepath.isEmpty()
            ? QStringLiteral("Unbenannt")
            : QFileInfo(filepath).fileName();
    }

    QString displayTitle() const
    {
        return (isModified() ? QStringLiteral("* ") : QString()) + baseName();
    }

    void detectEol(const QString &text)
    {
        QsciScintilla::EolMode mode = QsciScintilla::EolUnix;
        if (text.contains(QLatin1String("\r\n")))
            mode = QsciScintilla::EolWindows;
        else if (text.contains(QLatin1Char('\r')))
            mode = QsciScintilla::EolMac;
        setEolMode(mode);
    }

    static QString eolName(QsciScintilla::EolMode mode)
    {
        switch (mode) {
        case QsciScintilla::EolWindows: return QStringLiteral("Windows");
        case QsciScintilla::EolMac:     return QStringLiteral("Mac");
        default:                        return QStringLiteral("Unix");
        }
    }
};

// ---------------------------------------------------------------------------
// Find/Replace (counterpart of FindReplaceDlg in the original)
// ---------------------------------------------------------------------------

class FindReplaceDialog : public QDialog
{
public:
    explicit FindReplaceDialog(MainWindow *main);

    QLineEdit *editFind;
    QLineEdit *editReplace;
    QCheckBox *checkCase;
    QCheckBox *checkWord;
    QCheckBox *checkRegex;

    void findFrom(bool backward);
    void replaceOne();
    void replaceAll();

private:
    MainWindow *main;
};

// ---------------------------------------------------------------------------
// Markdown renderer and PDF output (own implementation, no external
// dependencies; the HTML targets the rich-text rendering of QTextDocument)
// ---------------------------------------------------------------------------

static QString mdEscape(const QString &s)
{
    QString r(s);
    r.replace(QLatin1Char('&'), QLatin1String("&amp;"));
    r.replace(QLatin1Char('<'), QLatin1String("&lt;"));
    r.replace(QLatin1Char('>'), QLatin1String("&gt;"));
    r.replace(QLatin1Char('"'), QLatin1String("&quot;"));
    return r;
}

// Fence bodies are HTML-escaped for display; revert before the executor
// writes the script to a file or pipes it to stdin. &amp; must run last.
static QString mdUnescape(const QString &s)
{
    QString r(s);
    r.replace(QLatin1String("&lt;"), QLatin1String("<"));
    r.replace(QLatin1String("&gt;"), QLatin1String(">"));
    r.replace(QLatin1String("&quot;"), QLatin1String("\""));
    r.replace(QLatin1String("&amp;"), QLatin1String("&"));
    return r;
}

// Studio palettes for the whole application (Fusion style + QSS below).
// Both themes share one idea: a quiet workbench surface, warm dark graphite
// or warm paper, with a single amber accent for the caret/active tab and a
// calm blue for links and selection.
static QPalette studioPalette(bool darkTheme)
{
    // Gruvbox studio: warm dark graphite (dark) / warm paper (light), one
    // yellow accent for caret and active tab, gruvbox blue and aqua for
    // links and run controls. Surfaces: bg0 chrome, bg1 base, bg2 raised.
    QPalette pal;
    if (darkTheme) {
        const QColor window("#1d2021"), text("#ebdbb2");
        pal.setColor(QPalette::Window, window);
        pal.setColor(QPalette::WindowText, text);
        pal.setColor(QPalette::Base, QColor("#282828"));
        pal.setColor(QPalette::AlternateBase, QColor("#32302f"));
        pal.setColor(QPalette::Text, text);
        pal.setColor(QPalette::Button, QColor("#32302f"));
        pal.setColor(QPalette::ButtonText, text);
        pal.setColor(QPalette::ToolTipBase, QColor("#32302f"));
        pal.setColor(QPalette::ToolTipText, text);
        pal.setColor(QPalette::Highlight, QColor("#504945"));
        pal.setColor(QPalette::HighlightedText, QColor("#fbf1c7"));
        pal.setColor(QPalette::Link, QColor("#83a598"));
        pal.setColor(QPalette::LinkVisited, QColor("#83a598"));
        pal.setColor(QPalette::Disabled, QPalette::Text, QColor("#665c54"));
        pal.setColor(QPalette::Disabled, QPalette::WindowText,
                     QColor("#665c54"));
        pal.setColor(QPalette::Disabled, QPalette::ButtonText,
                     QColor("#665c54"));
        return pal;
    }
    pal.setColor(QPalette::Window, QColor("#f2ecdc"));
    pal.setColor(QPalette::WindowText, QColor("#3c3836"));
    pal.setColor(QPalette::Base, QColor("#fdfaf2"));
    pal.setColor(QPalette::AlternateBase, QColor("#eee5cf"));
    pal.setColor(QPalette::Text, QColor("#3c3836"));
    pal.setColor(QPalette::Button, QColor("#eee5cf"));
    pal.setColor(QPalette::ButtonText, QColor("#3c3836"));
    pal.setColor(QPalette::ToolTipBase, QColor("#eee5cf"));
    pal.setColor(QPalette::ToolTipText, QColor("#3c3836"));
    pal.setColor(QPalette::Highlight, QColor("#d5c4a1"));
    pal.setColor(QPalette::HighlightedText, QColor("#282828"));
    pal.setColor(QPalette::Link, QColor("#076678"));
    pal.setColor(QPalette::LinkVisited, QColor("#076678"));
    pal.setColor(QPalette::Disabled, QPalette::Text, QColor("#a89984"));
    pal.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#a89984"));
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#a89984"));
    return pal;
}

static QString appStyleSheet(bool darkTheme)
{
    // Full chrome: flat surfaces, 1px hairlines, 4px radii, yellow accent for
    // focus and the active tab, gruvbox grays for hover/checked states.
    if (darkTheme) {
        return QStringLiteral(
            "QMenuBar { background: #1d2021; color: #a89984; }"
            "QMenuBar::item { padding: 4px 10px; background: transparent;"
            " border-radius: 4px; }"
            "QMenuBar::item:selected { background: #32302f; color: #ebdbb2; }"
            "QMenu { background: #282828; border: 1px solid #3c3836;"
            " border-radius: 6px; padding: 6px; color: #ebdbb2; }"
            "QMenu::item { padding: 6px 24px; border-radius: 4px; }"
            "QMenu::item:selected { background: #504945;"
            " color: #fbf1c7; }"
            "QMenu::item:disabled { color: #665c54; }"
            "QMenu::separator { height: 1px; background: #3c3836;"
            " margin: 6px 8px; }"
            "QToolBar { border: none; padding: 4px 8px; spacing: 4px;"
            " background: #1d2021; }"
            "QToolBar::separator { background: #3c3836; width: 1px;"
            " margin: 6px 4px; }"
            "QToolButton { background: transparent;"
            " border: 1px solid transparent; border-radius: 4px;"
            " padding: 4px 8px; color: #a89984; }"
            "QToolButton:hover { background: #32302f;"
            " border-color: #3c3836; color: #ebdbb2; }"
            "QToolButton:pressed, QToolButton:checked {"
            " background: #504945; color: #fbf1c7; }"
            "QPushButton { background: #32302f; border: 1px solid #3c3836;"
            " border-radius: 4px; padding: 5px 14px; color: #ebdbb2; }"
            "QPushButton:hover { background: #504945; color: #fbf1c7; }"
            "QPushButton:pressed { background: #282828; }"
            "QPushButton:disabled { color: #665c54; background: #282828; }"
            "QLineEdit, QPlainTextEdit, QComboBox, QSpinBox, QDoubleSpinBox {"
            " background: #282828; border: 1px solid #3c3836;"
            " border-radius: 4px; padding: 4px 8px; color: #ebdbb2;"
            " selection-background-color: #504945; }"
            "QLineEdit:focus, QPlainTextEdit:focus, QComboBox:focus,"
            " QSpinBox:focus, QDoubleSpinBox:focus {"
            " border-color: #fabd2f; }"
            "QComboBox::drop-down { border: none; width: 20px; }"
            "QMainWindow, QTabWidget { background: #1d2021; }"
            "QTabWidget::pane { border: 1px solid #3c3836;"
            " background: #1d2021; }"
            "QTabBar { background: #1d2021; }"
            "QTabBar::tab { padding: 6px 14px; background: transparent;"
            " color: #a89984; border: 1px solid transparent;"
            " border-bottom: 1px solid #3c3836; margin-right: 2px;"
            " padding-bottom: 5px; }"
            "QTabBar::tab:hover { background: #282828; color: #ebdbb2; }"
            "QTabBar::tab:selected { background: #282828; color: #fbf1c7;"
            " border: 1px solid #3c3836; border-bottom: 2px solid #fabd2f;"
            " border-top-left-radius: 4px; border-top-right-radius: 4px; }"
            "QStatusBar { background: #1d2021; color: #a89984;"
            " border-top: 1px solid #3c3836; }"
            "QDockWidget::title { padding: 6px 10px; background: #32302f;"
            " border-bottom: 1px solid #3c3836; color: #a89984; }"
            "QMainWindow::separator { background: #3c3836;"
            " width: 2px; height: 2px; }"
            "QScrollBar:vertical { background: #1d2021; width: 10px;"
            " margin: 2px; }"
            "QScrollBar::handle:vertical { background: #504945;"
            " border-radius: 4px; min-height: 24px; }"
            "QScrollBar::handle:vertical:hover { background: #665c54; }"
            "QScrollBar:horizontal { background: #1d2021; height: 10px;"
            " margin: 2px; }"
            "QScrollBar::handle:horizontal { background: #504945;"
            " border-radius: 4px; min-width: 24px; }"
            "QScrollBar::handle:horizontal:hover { background: #665c54; }"
            "QScrollBar::add-line, QScrollBar::sub-line {"
            " height: 0px; width: 0px; }"
            "QScrollBar::add-page, QScrollBar::sub-page {"
            " background: transparent; }"
            "QToolTip { background: #32302f; color: #ebdbb2;"
            " border: 1px solid #504945; padding: 4px 8px; }"
            "QCheckBox::indicator, QRadioButton::indicator {"
            " width: 13px; height: 13px; border: 1px solid #504945;"
            " border-radius: 3px; background: #282828; }"
            "QRadioButton::indicator { border-radius: 7px; }"
            "QCheckBox::indicator:checked, QRadioButton::indicator:checked {"
            " background: #fabd2f; border-color: #fabd2f; }"
            "QCheckBox::indicator:disabled, QRadioButton::indicator:disabled {"
            " border-color: #3c3836; background: #282828; }"
            "QHeaderView::section { background: #282828; color: #a89984;"
            " border: none; border-right: 1px solid #3c3836;"
            " border-bottom: 1px solid #3c3836; padding: 4px 8px; }"
            "QTreeWidget, QTreeView, QListView, QListWidget {"
            " border: 1px solid #3c3836; background: #282828; }"
            "QTreeWidget::item:selected, QTreeView::item:selected,"
            " QListWidget::item:selected, QListView::item:selected {"
            " background: #504945; color: #fbf1c7; }"
            "QGroupBox { border: 1px solid #3c3836; border-radius: 4px;"
            " margin-top: 10px; padding-top: 10px; }"
            "QGroupBox::title { color: #a89984; subcontrol-origin: margin;"
            " left: 10px; padding: 0 4px; }"
            "QTextEdit, QTextBrowser { border: none; background: #282828; }"
            "QSplitter::handle { background: #1d2021; }");
    }
    return QStringLiteral(
        "QMenuBar { background: #f2ecdc; color: #7c6f64; }"
        "QMenuBar::item { padding: 4px 10px; background: transparent;"
        " border-radius: 4px; }"
        "QMenuBar::item:selected { background: #eee5cf; color: #3c3836; }"
        "QMenu { background: #fdfaf2; border: 1px solid #d5c4a1;"
        " border-radius: 6px; padding: 6px; color: #3c3836; }"
        "QMenu::item { padding: 6px 24px; border-radius: 4px; }"
        "QMenu::item:selected { background: #d5c4a1; color: #282828; }"
        "QMenu::item:disabled { color: #a89984; }"
        "QMenu::separator { height: 1px; background: #d5c4a1;"
        " margin: 6px 8px; }"
        "QToolBar { border: none; padding: 4px 8px; spacing: 4px;"
        " background: #f2ecdc; }"
        "QToolBar::separator { background: #d5c4a1; width: 1px;"
        " margin: 6px 4px; }"
        "QToolButton { background: transparent;"
        " border: 1px solid transparent; border-radius: 4px;"
        " padding: 4px 8px; color: #7c6f64; }"
        "QToolButton:hover { background: #eee5cf;"
        " border-color: #d5c4a1; color: #3c3836; }"
        "QToolButton:pressed, QToolButton:checked {"
        " background: #d5c4a1; color: #282828; }"
        "QPushButton { background: #eee5cf; border: 1px solid #d5c4a1;"
        " border-radius: 4px; padding: 5px 14px; color: #3c3836; }"
        "QPushButton:hover { background: #d5c4a1; color: #282828; }"
        "QPushButton:pressed { background: #c9b68e; }"
        "QPushButton:disabled { color: #a89984; background: #f2ecdc; }"
        "QLineEdit, QPlainTextEdit, QComboBox, QSpinBox, QDoubleSpinBox {"
        " background: #fdfaf2; border: 1px solid #d5c4a1;"
        " border-radius: 4px; padding: 4px 8px; color: #3c3836;"
        " selection-background-color: #d5c4a1; }"
        "QLineEdit:focus, QPlainTextEdit:focus, QComboBox:focus,"
        " QSpinBox:focus, QDoubleSpinBox:focus { border-color: #b57614; }"
        "QComboBox::drop-down { border: none; width: 20px; }"
        "QMainWindow, QTabWidget { background: #f2ecdc; }"
        "QTabWidget::pane { border: 1px solid #d5c4a1;"
        " background: #f2ecdc; }"
        "QTabBar { background: #f2ecdc; }"
        "QTabBar::tab { padding: 6px 14px; background: transparent;"
        " color: #7c6f64; border: 1px solid transparent;"
        " border-bottom: 1px solid #d5c4a1; margin-right: 2px;"
        " padding-bottom: 5px; }"
        "QTabBar::tab:hover { background: #fdfaf2; color: #3c3836; }"
        "QTabBar::tab:selected { background: #fdfaf2; color: #282828;"
        " border: 1px solid #d5c4a1; border-bottom: 2px solid #b57614;"
        " border-top-left-radius: 4px; border-top-right-radius: 4px; }"
        "QStatusBar { background: #f2ecdc; color: #7c6f64;"
        " border-top: 1px solid #d5c4a1; }"
        "QDockWidget::title { padding: 6px 10px; background: #eee5cf;"
        " border-bottom: 1px solid #d5c4a1; color: #7c6f64; }"
        "QMainWindow::separator { background: #d5c4a1;"
        " width: 2px; height: 2px; }"
        "QScrollBar:vertical { background: #f2ecdc; width: 10px;"
        " margin: 2px; }"
        "QScrollBar::handle:vertical { background: #c9b68e;"
        " border-radius: 4px; min-height: 24px; }"
        "QScrollBar::handle:vertical:hover { background: #a89984; }"
        "QScrollBar:horizontal { background: #f2ecdc; height: 10px;"
        " margin: 2px; }"
        "QScrollBar::handle:horizontal { background: #c9b68e;"
        " border-radius: 4px; min-width: 24px; }"
        "QScrollBar::handle:horizontal:hover { background: #a89984; }"
        "QScrollBar::add-line, QScrollBar::sub-line {"
        " height: 0px; width: 0px; }"
        "QScrollBar::add-page, QScrollBar::sub-page {"
        " background: transparent; }"
        "QToolTip { background: #eee5cf; color: #3c3836;"
        " border: 1px solid #bdae93; padding: 4px 8px; }"
        "QCheckBox::indicator, QRadioButton::indicator {"
        " width: 13px; height: 13px; border: 1px solid #bdae93;"
        " border-radius: 3px; background: #fdfaf2; }"
        "QRadioButton::indicator { border-radius: 7px; }"
        "QCheckBox::indicator:checked, QRadioButton::indicator:checked {"
        " background: #b57614; border-color: #b57614; }"
        "QCheckBox::indicator:disabled, QRadioButton::indicator:disabled {"
        " border-color: #d5c4a1; background: #fdfaf2; }"
        "QHeaderView::section { background: #f2ecdc; color: #7c6f64;"
        " border: none; border-right: 1px solid #d5c4a1;"
        " border-bottom: 1px solid #d5c4a1; padding: 4px 8px; }"
        "QTreeWidget, QTreeView, QListView, QListWidget {"
        " border: 1px solid #d5c4a1; background: #fdfaf2; }"
        "QTreeWidget::item:selected, QTreeView::item:selected,"
        " QListWidget::item:selected, QListView::item:selected {"
        " background: #d5c4a1; color: #282828; }"
        "QGroupBox { border: 1px solid #d5c4a1; border-radius: 4px;"
        " margin-top: 10px; padding-top: 10px; }"
        "QGroupBox::title { color: #7c6f64; subcontrol-origin: margin;"
        " left: 10px; padding: 0 4px; }"
        "QTextEdit, QTextBrowser { border: none; background: #fdfaf2; }"
        "QSplitter::handle { background: #f2ecdc; }");
}

// Preview theme flag: the Markdown renderer produces small inline styles
// (code-block headers, run links) that must follow the active theme.
static bool gPreviewDark = false;
static void setPreviewDark(bool dark) { gPreviewDark = dark; }
static QString mdBarBg()  { return gPreviewDark ? QStringLiteral("#1d2021") : QStringLiteral("#eee5cf"); }
static QString mdBarFg()  { return gPreviewDark ? QStringLiteral("#a89984") : QStringLiteral("#7c6f64"); }
static QString mdAccent() { return gPreviewDark ? QStringLiteral("#8ec07c") : QStringLiteral("#427b58"); }
static QString mdOutHead(){ return gPreviewDark ? QStringLiteral("#fe8019") : QStringLiteral("#af3a03"); }

struct RunBlock {
    QString lang;   // normalized: "c", "cpp", "python", "sql" (empty: not runnable)
    QString code;   // raw code inside the fence
};

static QString runBlockKey(const QString &lang, const QString &code)
{
    return lang + QChar(1) + code;
}

static QString normalizeRunLang(const QString &info)
{
    const QString f = info.section(QChar(' '), 0, 0).toLower();
    if (f == QLatin1String("c"))
        return QStringLiteral("c");
    if (f == QLatin1String("cpp") || f == QLatin1String("c++")
        || f == QLatin1String("cxx") || f == QLatin1String("cc"))
        return QStringLiteral("cpp");
    if (f == QLatin1String("python") || f == QLatin1String("py")
        || f == QLatin1String("python3"))
        return QStringLiteral("python");
    if (f == QLatin1String("sql") || f == QLatin1String("sqlite")
        || f == QLatin1String("sqlite3"))
        return QStringLiteral("sql");
    return QString();
}

static QString runLanguageLabel(const QString &lang)
{
    if (lang == QLatin1String("c"))      return QStringLiteral("C");
    if (lang == QLatin1String("cpp"))    return QStringLiteral("C++");
    if (lang == QLatin1String("python"))return QStringLiteral("Python");
    if (lang == QLatin1String("sql"))   return QStringLiteral("SQL");
    return QString();
}

// Program arguments per language (phase 0: compile for C/C++, direct run for
// python/sql). Pure function, white-box-testable.
static QStringList executorArgs(const QString &lang, const QString &scriptPath,
                                const QString &binPath)
{
    if (lang == QLatin1String("python"))
        return QStringList{scriptPath};
    if (lang == QLatin1String("sql"))
        return QStringList{QStringLiteral(":memory:")};
    if (lang == QLatin1String("c") || lang == QLatin1String("cpp"))
        return QStringList{scriptPath, QStringLiteral("-o"), binPath};
    return QStringList();
}

static QString defaultExecutor(const QString &lang)
{
    if (lang == QLatin1String("python")) return QStringLiteral("/usr/bin/python3");
    if (lang == QLatin1String("sql"))    return QStringLiteral("/usr/bin/sqlite3");
    if (lang == QLatin1String("cpp"))    return QStringLiteral("/usr/bin/g++");
    return QStringLiteral("/usr/bin/gcc");
}

// sqlite3 reads the script from stdin; headers/column mode for readable output.
static QString sqliteStdinScript(const QString &sql)
{
    return QStringLiteral(".headers on\n.mode column\n") + sql
           + QLatin1Char('\n');
}

static QString formatRunResult(int exitCode, const QString &output,
                               qint64 elapsedMs, bool timedOut,
                               const QString &phase)
{
    QString head;
    if (timedOut)
        head = QStringLiteral("Zeitlimit \u00fcberschritten (10 s) - "
                              "Ausf\u00fchrung abgebrochen");
    else
        head = QStringLiteral("Exit %1 (%2 s)")
                   .arg(exitCode)
                   .arg(QString::number(elapsedMs / 1000.0, 'f', 2));
    if (!phase.isEmpty())
        head.prepend(phase + QStringLiteral(": "));
    const QString body = output.isEmpty()
                             ? QStringLiteral("(keine Ausgabe)")
                             : output;
    return head + QLatin1Char('\n') + body;
}

struct RunJob {
    QProcess *proc = nullptr;
    QTimer *killer = nullptr;
    QTemporaryDir *dir = nullptr;
    QElapsedTimer clock;
    int phase = -1;          // 0 = compile (c/cpp), 1 = run binary, 2 = direct
    QString lang;
    QString key;
    QString binPath;
    QString output;
    bool timedOut = false;
    bool capped = false;
};

// ---------------------------------------------------------------------------
// Lightweight inline TeX subset (Unicode approximation) for the Markdown
// preview and the PDF export. No external tools: $...$, $$...$$, \(...\) and
// \[...\] are converted to styled Unicode text (Greek letters, operators,
// fractions, roots, sub-/superscripts, \mathbb/\mathbf/\text, common
// function names). A pragmatic subset, not a full LaTeX engine.
// ---------------------------------------------------------------------------

static QString texSymbol(const QString &cmd)
{
    static const struct { const char *name; ushort sym; } M[] = {
        // Greek letters
        {"alpha", 0x03b1}, {"beta", 0x03b2}, {"gamma", 0x03b3},
        {"delta", 0x03b4}, {"epsilon", 0x03b5}, {"varepsilon", 0x03b5},
        {"zeta", 0x03b6}, {"eta", 0x03b7}, {"theta", 0x03b8},
        {"vartheta", 0x03d1}, {"iota", 0x03b9}, {"kappa", 0x03ba},
        {"lambda", 0x03bb}, {"mu", 0x03bc}, {"nu", 0x03bd}, {"xi", 0x03be},
        {"pi", 0x03c0}, {"rho", 0x03c1}, {"sigma", 0x03c3}, {"tau", 0x03c4},
        {"upsilon", 0x03c5}, {"phi", 0x03c6}, {"varphi", 0x03d5},
        {"chi", 0x03c7}, {"psi", 0x03c8}, {"omega", 0x03c9},
        {"Gamma", 0x0393}, {"Delta", 0x0394}, {"Theta", 0x0398},
        {"Lambda", 0x039b}, {"Xi", 0x039e}, {"Pi", 0x03a0}, {"Sigma", 0x03a3},
        {"Upsilon", 0x03a5}, {"Phi", 0x03a6}, {"Psi", 0x03a8}, {"Omega", 0x03a9},
        // Relation and binary operators
        {"pm", 0x00b1}, {"mp", 0x2213}, {"times", 0x00d7}, {"div", 0x00f7},
        {"cdot", 0x22c5}, {"ast", 0x2217}, {"star", 0x22c6}, {"circ", 0x2218},
        {"bullet", 0x2219}, {"oplus", 0x2295}, {"ominus", 0x2296},
        {"otimes", 0x2297}, {"cup", 0x222a}, {"cap", 0x2229},
        {"setminus", 0x2216}, {"leq", 0x2264}, {"le", 0x2264},
        {"geq", 0x2265}, {"ge", 0x2265}, {"neq", 0x2260}, {"ne", 0x2260},
        {"approx", 0x2248}, {"equiv", 0x2261}, {"sim", 0x223c},
        {"simeq", 0x2243}, {"propto", 0x221d}, {"ll", 0x226a}, {"gg", 0x226b},
        {"prec", 0x227a}, {"succ", 0x227b}, {"subset", 0x2282},
        {"subseteq", 0x2286}, {"supset", 0x2283}, {"supseteq", 0x2287},
        {"elem", 0x2208}, {"in", 0x2208}, {"notin", 0x2209}, {"ni", 0x220b},
        {"emptyset", 0x2205}, {"varnothing", 0x2205},
        {"perp", 0x22a5}, {"parallel", 0x2225}, {"angle", 0x2220},
        {"wedge", 0x2227}, {"vee", 0x2228}, {"neg", 0x00ac},
        {"Rightarrow", 0x21d2}, {"Leftarrow", 0x21d0},
        {"Leftrightarrow", 0x21d4}, {"leftrightarrow", 0x2194},
        {"rightarrow", 0x2192}, {"to", 0x2192}, {"leftarrow", 0x2190},
        {"longrightarrow", 0x27f6}, {"mapsto", 0x21a6},
        {"uparrow", 0x2191}, {"downarrow", 0x2193}, {"updownarrow", 0x2195},
        // Calculus and large operators
        {"int", 0x222b}, {"iint", 0x222c}, {"iiint", 0x222d},
        {"oint", 0x222e}, {"sum", 0x2211}, {"prod", 0x220f},
        {"bigcup", 0x22c3}, {"bigcap", 0x22c2},
        {"partial", 0x2202}, {"nabla", 0x2207}, {"infty", 0x221e},
        {"hbar", 0x210f}, {"ell", 0x2113}, {"exists", 0x2203},
        {"nexists", 0x2204},
        // Misc
        {"ldots", 0x2026}, {"cdots", 0x22ef}, {"vdots", 0x22ee},
        {"ddots", 0x22f1}, {"dots", 0x2026}, {"prime", 0x2032},
        {"deg", 0x00b0}, {"dagger", 0x2020}, {"ddagger", 0x2021},
        {"flat", 0x266d}, {"natural", 0x266e}, {"sharp", 0x266f},
    };
    for (const auto &e : M) {
        const QString name = QString::fromLatin1(e.name);
        if (cmd == name)
            return QString(QChar(e.sym));
    }
    return QString();
}

// Operator names typeset upright (not italic variables).
static bool texIsFunctionName(const QString &cmd)
{
    static const char *const N[] = {
        "sin", "cos", "tan", "cot", "sec", "csc", "arcsin", "arccos", "arctan",
        "sinh", "cosh", "tanh", "coth", "ln", "log", "lg", "exp", "lim",
        "max", "min", "sup", "inf", "det", "dim", "ker", "gcd", "arg",
        "Hom", "Pr", "mod", "bmod",
    };
    for (const char *n : N) {
        if (cmd == QLatin1String(n))
            return true;
    }
    return false;
}

// Parses one atom (single char, braced group, or command with arguments)
// starting at index i and advances i. srcPlain receives the source text of
// plain atoms (used by \frac for bracket decisions). Returns HTML.
static QString texParseAtom(const QString &tex, int &i, QString *srcPlain);

// Parses a sequence until end or the stop character (stop not consumed).
static QString texParseSeq(const QString &tex, int &i, QChar stop)
{
    const int n = tex.size();
    QString out;
    while (i < n) {
        const QChar c = tex.at(i);
        if (!stop.isNull() && c == stop)
            break;
        if (c == QLatin1Char('^') || c == QLatin1Char('_')) {
            ++i;
            QString arg;
            if (i < n && (tex.at(i) == QLatin1Char('{')
                          || tex.at(i) == QLatin1Char('\\'))) {
                // Braced group or control word (e.g. ^\infty): the atom
                // parser returns HTML-safe output and consumes the atom.
                QString plain;
                const int before = i;
                arg = texParseAtom(tex, i, &plain);
                if (i == before) { // safety fallback: single token
                    arg = QString(tex.at(i)).toHtmlEscaped();
                    ++i;
                }
            } else if (i < n) {
                // Single token, TeX style.
                arg = QString(tex.at(i)).toHtmlEscaped();
                ++i;
            }
            out += QStringLiteral("<%1>%2</%1>")
                       .arg(c == QLatin1Char('^') ? QStringLiteral("sup")
                                                   : QStringLiteral("sub"),
                            arg);
            continue;
        }
        const int before = i;
        QString plain;
        out += texParseAtom(tex, i, &plain);
        if (i == before) { // safety against endless loops
            out += QString(c).toHtmlEscaped();
            ++i;
        }
    }
    return out;
}

static QString texParseAtom(const QString &tex, int &i, QString *srcPlain)
{
    const int n = tex.size();
    if (srcPlain)
        srcPlain->clear();
    if (i >= n)
        return QString();

    const QChar c = tex.at(i);

    if (c == QLatin1Char('{')) {
        ++i; // consume '{'; the group body starts here
        const int start = i;
        const QString body = texParseSeq(tex, i, QChar('}'));
        if (i < n && tex.at(i) == QLatin1Char('}'))
            ++i;
        if (srcPlain)
            *srcPlain = tex.mid(start, qMax(0, i - start - 1));
        return body;
    }
    if (c == QLatin1Char('\\')) {
        ++i;
        if (i >= n)
            return QLatin1String("\\");
        const QChar c2 = tex.at(i);
        if (c2 == QLatin1Char('{') || c2 == QLatin1Char('}')
            || c2 == QLatin1Char('|')) {
            ++i;
            if (srcPlain)
                *srcPlain = QString(c2);
            return QString(c2);
        }
        if (c2 == QLatin1Char('\\')) {
            ++i;
            return QStringLiteral("<br>");
        }
        if (c2 == QLatin1Char(' ') || c2 == QLatin1Char(';')
            || c2 == QLatin1Char(':') || c2 == QLatin1Char(',')) {
            ++i;
            return QStringLiteral(" ");
        }
        if (c2 == QLatin1Char('!')) {
            ++i;
            return QString();
        }
        if (!c2.isLetter()) {
            ++i;
            if (srcPlain)
                *srcPlain = QString(c2);
            return QString(c2).toHtmlEscaped();
        }
        int j = i;
        while (j < n && tex.at(j).isLetter())
            ++j;
        const QString cmd = tex.mid(i, j - i);
        i = j;

        // Fractions: \frac{a}{b} (and \dfrac, \tfrac)
        if (cmd == QLatin1String("frac") || cmd == QLatin1String("dfrac")
            || cmd == QLatin1String("tfrac")) {
            if (i < n && tex.at(i) == QLatin1Char('{')) {
                QString srcA, srcB;
                const QString a = texParseAtom(tex, i, &srcA);
                if (i < n && tex.at(i) == QLatin1Char('{')) {
                    const QString b = texParseAtom(tex, i, &srcB);
                    const bool wa = srcA.trimmed().size() > 1;
                    const bool wb = srcB.trimmed().size() > 1;
                    return (wa ? QStringLiteral("(") : QString()) + a
                         + (wa ? QStringLiteral(")") : QString())
                         + QStringLiteral("\u2044")
                         + (wb ? QStringLiteral("(") : QString()) + b
                         + (wb ? QStringLiteral(")") : QString());
                }
                return a;
            }
            return QString();
        }
        // Root: \sqrt[n]{x}
        if (cmd == QLatin1String("sqrt")) {
            QString idx;
            if (i < n && tex.at(i) == QLatin1Char('[')) {
                const int close = tex.indexOf(QLatin1Char(']'), i);
                if (close > i) {
                    idx = tex.mid(i + 1, close - i - 1);
                    i = close + 1;
                }
            }
            if (i < n && tex.at(i) == QLatin1Char('{')) {
                QString srcX;
                const QString x = texParseAtom(tex, i, &srcX);
                const bool w = srcX.trimmed().size() > 1;
                const QString sup = idx.isEmpty()
                    ? QString()
                    : QStringLiteral("<sup>%1</sup>")
                          .arg(QString(idx).toHtmlEscaped());
                return sup + QStringLiteral("\u221a")
                     + (w ? QStringLiteral("(") : QString()) + x
                     + (w ? QStringLiteral(")") : QString());
            }
            return QStringLiteral("\u221a");
        }
        // Upright plain text
        if (cmd == QLatin1String("text") || cmd == QLatin1String("textrm")
            || cmd == QLatin1String("mathrm")) {
            if (i < n && tex.at(i) == QLatin1Char('{')) {
                int depth = 0, close = -1;
                for (int k = i; k < n; ++k) {
                    if (tex.at(k) == QLatin1Char('{'))
                        ++depth;
                    else if (tex.at(k) == QLatin1Char('}')) {
                        --depth;
                        if (depth == 0) { close = k; break; }
                    }
                }
                if (close > i) {
                    const QString raw = tex.mid(i + 1, close - i - 1);
                    i = close + 1;
                    return QStringLiteral(
                               "<span style=\"font-style:normal\">%1</span>")
                        .arg(raw.toHtmlEscaped());
                }
            }
            return QString();
        }
        if (cmd == QLatin1String("mathbf") || cmd == QLatin1String("bm")
            || cmd == QLatin1String("boldsymbol")) {
            if (i < n && tex.at(i) == QLatin1Char('{')) {
                QString plain;
                const QString body = texParseAtom(tex, i, &plain);
                return QStringLiteral("<b>%1</b>").arg(body);
            }
            return QString();
        }
        if (cmd == QLatin1String("mathbb")) {
            if (i < n && tex.at(i) == QLatin1Char('{')) {
                int depth = 0, close = -1;
                for (int k = i; k < n; ++k) {
                    if (tex.at(k) == QLatin1Char('{'))
                        ++depth;
                    else if (tex.at(k) == QLatin1Char('}')) {
                        --depth;
                        if (depth == 0) { close = k; break; }
                    }
                }
                if (close > i) {
                    const QString body = tex.mid(i + 1, close - i - 1);
                    i = close + 1;
                    static const struct { ushort ch; uint sym; } BB[] = {
                        {'C', 0x2102}, {'H', 0x210d}, {'N', 0x2115},
                        {'P', 0x2119}, {'Q', 0x211a}, {'R', 0x211d},
                        {'Z', 0x2124},
                    };
                    QString out;
                    for (const QChar &ch : body) {
                        bool mapped = false;
                        for (const auto &e : BB) {
                            if (ch.unicode() == e.ch) {
                                out += QString(QChar(e.sym));
                                mapped = true;
                                break;
                            }
                        }
                        if (!mapped)
                            out += ch;
                    }
                    return out;
                }
            }
            return QString();
        }
        // Layout noise that is consumed silently
        if (cmd == QLatin1String("left") || cmd == QLatin1String("right")
            || cmd == QLatin1String("big") || cmd == QLatin1String("Big")
            || cmd == QLatin1String("bigg") || cmd == QLatin1String("Bigg")
            || cmd == QLatin1String("bigl") || cmd == QLatin1String("bigr")
            || cmd == QLatin1String("Bigl") || cmd == QLatin1String("Bigr")
            || cmd == QLatin1String("bigm") || cmd == QLatin1String("limits")
            || cmd == QLatin1String("nolimits")
            || cmd == QLatin1String("displaystyle"))
            return QString();
        if (cmd == QLatin1String("begin") || cmd == QLatin1String("end")) {
            if (i < n && tex.at(i) == QLatin1Char('{')) {
                const int close = tex.indexOf(QLatin1Char('}'), i);
                if (close > i)
                    i = close + 1;
            }
            return QString();
        }
        if (cmd == QLatin1String("quad"))
            return QStringLiteral("\u2003");
        if (cmd == QLatin1String("qquad"))
            return QStringLiteral("\u2003\u2003");
        if (texIsFunctionName(cmd))
            return QStringLiteral(
                       "<span style=\"font-style:normal\">%1</span>")
                .arg(cmd);
        const QString sym = texSymbol(cmd);
        if (!sym.isEmpty())
            return sym;
        // Unknown command: keep it visible (honest pass-through)
        return QStringLiteral(
                   "<span style=\"font-style:normal\">\\%1</span>")
            .arg(cmd.toHtmlEscaped());
    }
    if (c == QLatin1Char('}')) {
        ++i;
        return QStringLiteral("}");
    }
    if (c == QLatin1Char('&')) {
        ++i;
        return QStringLiteral(" ");
    }
    if (c == QLatin1Char(' ')) {
        ++i;
        return QStringLiteral(" ");
    }
    if (c.isDigit()) {
        int j = i;
        while (j < n && (tex.at(j).isDigit()
                         || tex.at(j) == QLatin1Char('.')
                         || tex.at(j) == QLatin1Char(',')))
            ++j;
        const QString num = tex.mid(i, j - i);
        i = j;
        if (srcPlain)
            *srcPlain = num;
        return QStringLiteral("<span style=\"font-style:normal\">%1</span>")
            .arg(num);
    }
    if (c.isLetter()) {
        ++i;
        if (srcPlain)
            *srcPlain = QString(c);
        return QStringLiteral("<i>%1</i>").arg(QString(c));
    }
    ++i;
    if (srcPlain)
        *srcPlain = QString(c);
    return QString(c).toHtmlEscaped();
}

// Inline math span ($...$ / \(...\)).
static QString mathInlineToHtml(const QString &tex)
{
    int i = 0;
    return QStringLiteral("<span style=\"white-space:nowrap\">%1</span>")
        .arg(texParseSeq(tex, i, QChar()));
}

// Display math block ($$...$$ / \[...\]): own centered paragraph.
static QString mathDisplayToHtml(const QString &tex)
{
    int i = 0;
    return QStringLiteral(
               "<div align=\"center\" style=\"font-size:12pt\">%1</div>")
        .arg(texParseSeq(tex, i, QChar()));
}

// Inline elements: code spans are protected by placeholders first, then
// inline/display math is extracted, then images (rendered as a hint),
// links, bold, italic, strikethrough. Math is rendered as styled Unicode
// text (see the TeX subset engine above).
static QString mdInlinePass(QString s)
{
    QStringList codeSpans;
    {
        static const QRegularExpression re(
            QStringLiteral("(`+)(.*?)\\1"),
            QRegularExpression::DotMatchesEverythingOption);
        QRegularExpressionMatch m = re.match(s);
        for (int guard = 0; m.hasMatch() && guard < 400; ++guard) {
            codeSpans.append(mdEscape(m.captured(2)));
            s.replace(m.capturedStart(),
                      m.capturedEnd() - m.capturedStart(),
                      QStringLiteral("\x01CS%1\x02").arg(codeSpans.size() - 1));
            m = re.match(s);
        }
    }

    // Math: $...$, $$...$$ (single-line display), \(...\). Escaped dollars
    // are shielded first so "$5" prices stay untouched.
    QStringList mathParts;
    {
        s.replace(QStringLiteral("\\$"), QStringLiteral("\x01Q"));
        auto stash = [&s, &mathParts](const QRegularExpression &re,
                                       bool display) {
            QRegularExpressionMatch m = re.match(s);
            for (int guard = 0; m.hasMatch() && guard < 200; ++guard) {
                const QString body = m.captured(1);
                mathParts.append(display ? mathDisplayToHtml(body)
                                         : mathInlineToHtml(body));
                s.replace(m.capturedStart(),
                          m.capturedEnd() - m.capturedStart(),
                          QStringLiteral("\x01MT%1\x02")
                              .arg(mathParts.size() - 1));
                m = re.match(s);
            }
        };
        stash(QRegularExpression(
                  QStringLiteral("\\\\\\(([^\\n]+?)\\\\\\)")), false);
        stash(QRegularExpression(
                  QStringLiteral("\\$\\$([^\\n]+?)\\$\\$")), true);
        stash(QRegularExpression(
                  QStringLiteral("\\$([^\\$\\s][^\\$\\n]*?[^\\$\\s])\\$")),
              false);
        stash(QRegularExpression(
                  QStringLiteral("\\$([^\\$\\s])\\$")), false);
        s.replace(QStringLiteral("\x01Q"), QStringLiteral("$"));
    }

    s = mdEscape(s);

    if (s.contains(QStringLiteral("!["))) {
        static const QRegularExpression img(
            QStringLiteral("!\\[([^\\]]*)\\]\\(([^)\\s]+)\\)"));
        s.replace(img, QStringLiteral("<i>[Bild: \\1]</i>"));
    }
    if (s.contains(QLatin1String("]("))) {
        static const QRegularExpression link(
            QStringLiteral("\\[([^\\]]+)\\]\\(([^)\\s]+)\\)"));
        s.replace(link, QStringLiteral("<a href=\"\\2\">\\1</a>"));
    }
    if (s.contains(QLatin1String("http"))) {
        static const QRegularExpression autoLink(
            QStringLiteral("(^|\\s)(https?://[^\\s<]+)"));
        s.replace(autoLink, QStringLiteral("\\1<a href=\"\\2\">\\2</a>"));
    }
    s.replace(QRegularExpression(QStringLiteral("\\*\\*(.+?)\\*\\*")),
              QStringLiteral("<b>\\1</b>"));
    s.replace(QRegularExpression(QStringLiteral("__(.+?)__")),
              QStringLiteral("<b>\\1</b>"));
    s.replace(QRegularExpression(
                  QStringLiteral("\\*([^*\\s][^*]*?)\\*")),
              QStringLiteral("<i>\\1</i>"));
    s.replace(QRegularExpression(
                  QStringLiteral("(^|\\W)_([^_]+)_(?=\\W|$)")),
              QStringLiteral("\\1<i>\\2</i>"));
    s.replace(QRegularExpression(QStringLiteral("~~(.+?)~~")),
              QStringLiteral("<s>\\1</s>"));

    for (int i = 0; i < mathParts.size(); ++i)
        s.replace(QStringLiteral("\x01MT%1\x02").arg(i), mathParts.at(i));
    for (int i = 0; i < codeSpans.size(); ++i)
        s.replace(QStringLiteral("\x01CS%1\x02").arg(i),
                  QStringLiteral("<code>%1</code>").arg(codeSpans.at(i)));

    s.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
    return s;
}

static QString markdownToHtml(const QString &md,
                              QList<RunBlock> *blocks = nullptr,
                              const QHash<QString, QString> *results = nullptr,
                              const QString &runningKey = QString())
{
    QStringList lines;
    for (const QString &raw : md.split(QLatin1Char('\n'))) {
        QString ln(raw);
        while (!ln.isEmpty() && ln.endsWith(QLatin1Char('\r')))
            ln.chop(1);
        lines.append(ln);
    }

    QString out;
    QString para;
    QStringList quoteLines;
    QStringList tbl;
    QString codeBuf;
    QString codeLang;      // normalized run language of the open fence
    QString codeLabel;     // fence info string shown in the run bar
    QString mathBuf;
    bool inUl = false;
    bool inOl = false;
    bool inCode = false;
    bool inMathF = false;  // ```math fence
    bool inMathD = false;  // $$ ... $$ block
    bool inMathB = false;  // \[ ... \] block

    auto flushPara = [&]() {
        if (!para.isEmpty()) {
            out += QStringLiteral("<p>%1</p>\n").arg(mdInlinePass(para));
            para.clear();
        }
    };
    auto flushLists = [&]() {
        if (inUl) { out += QStringLiteral("</ul>\n"); inUl = false; }
        if (inOl) { out += QStringLiteral("</ol>\n"); inOl = false; }
    };
    auto flushQuote = [&]() {
        if (!quoteLines.isEmpty()) {
            out += QStringLiteral("<blockquote>%1</blockquote>\n")
                       .arg(markdownToHtml(quoteLines.join(QLatin1Char('\n')),
                                           nullptr));
            quoteLines.clear();
        }
    };
    auto flushCode = [&]() {
        if (!codeBuf.isEmpty() || inCode) {
            const QString lang = codeLang;
            const QString label = codeLabel;
            const bool runnable = !lang.isEmpty();
            const QString key = runBlockKey(lang, codeBuf);
            int id = -1;
            if (blocks && runnable) {
                RunBlock rb;
                rb.lang = lang;
                rb.code = codeBuf;
                blocks->append(rb);
                id = blocks->size() - 1;
            }
            if (blocks && runnable) {
                const bool running = (key == runningKey);
                const QString res = results ? results->value(key) : QString();
                out += QStringLiteral(
                           "<table width=\"100%%\" cellspacing=\"0\" "
                           "cellpadding=\"4\" bgcolor=\"%1\">"
                           "<tr><td align=\"left\">"
                           "<font color=\"%2\" style=\"font-size:9pt\">"
                           "<b>%3</b></font></td>"
                           "<td align=\"right\">"
                           "%4</td></tr></table>\n")
                           .arg(mdBarBg(), mdBarFg())
                           .arg(label.isEmpty() ? runLanguageLabel(lang)
                                                : label.toHtmlEscaped())
                           .arg(running
                                    ? QStringLiteral(
                                          "<font color=\"%1\" "
                                          "style=\"font-size:9pt\">"
                                          "wird ausgef\u00fchrt \u2026</font>")
                                          .arg(mdAccent())
                                    : QStringLiteral(
                                          "<a href=\"npplxrun:%1\" "
                                          "style=\"font-size:9pt\">"
                                          "\u25b6 Ausf\u00fchren</a>")
                                          .arg(id));
                if (!res.isEmpty()) {
                    out += QStringLiteral(
                               "<table width=\"100%%\" cellspacing=\"0\" "
                               "cellpadding=\"4\" bgcolor=\"%1\"><tr><td>")
                               .arg(mdBarBg())
                           + QStringLiteral(
                                 "<font color=\"%1\" style=\"font-size:9pt\">"
                                 "<b>%2</b></font></td></tr>"
                                 "<tr><td><pre style=\"font-size:9pt\">"
                                 "%3</pre></td></tr></table>\n")
                                 .arg(mdOutHead())
                                 .arg(QStringLiteral("Ergebnis"))
                                 .arg(mdEscape(res));
                }
                out += QStringLiteral("<pre><code>%1</code></pre>\n")
                           .arg(codeBuf);
            } else {
                out += QStringLiteral("<pre><code>%1</code></pre>\n")
                           .arg(codeBuf);
            }
            codeBuf.clear();
            inCode = false;
            codeLang.clear();
            codeLabel.clear();
        }
    };
    auto flushTable = [&]() {
        if (tbl.isEmpty())
            return;
        out += QStringLiteral(
                   "<table border=\"1\" cellspacing=\"0\" cellpadding=\"4\" "
                   "width=\"100%\">");
        for (int i = 0; i < tbl.size(); ++i) {
            QStringList cells = tbl.at(i).split(QLatin1Char('|'));
            if (!cells.isEmpty() && cells.first().trimmed().isEmpty())
                cells.removeFirst();
            if (!cells.isEmpty() && cells.last().trimmed().isEmpty())
                cells.removeLast();
            out += QStringLiteral("<tr>");
            const bool head = (i == 0);
            for (const QString &cell : cells) {
                const QString body = mdInlinePass(cell.trimmed());
                out += head ? QStringLiteral("<th>%1</th>").arg(body)
                            : QStringLiteral("<td>%1</td>").arg(body);
            }
            out += QStringLiteral("</tr>");
        }
        out += QStringLiteral("</table>\n");
        tbl.clear();
    };
    auto flushAll = [&]() {
        flushPara();
        flushLists();
        flushQuote();
        flushTable();
    };
    auto flushMath = [&]() {
        if (inMathF || inMathD || inMathB) {
            if (out.endsWith(QLatin1String("<br>")))
                out.chop(4);
            out += mathDisplayToHtml(mathBuf);
            mathBuf.clear();
            inMathF = inMathD = inMathB = false;
        }
    };

    static const QRegularExpression headingRe(
        QStringLiteral("^\\s{0,3}(#{1,6})\\s+(.*\\S)\\s*#*$"));
    static const QRegularExpression hrRe(
        QStringLiteral("^\\s{0,3}(?:(?:\\*\\s*){3,}|(?:-\\s*){3,}|"
                       "(?:_\\s*){3,})$"));
    static const QRegularExpression ulRe(
        QStringLiteral("^\\s*[-*+]\\s+(.*)$"));
    static const QRegularExpression olRe(
        QStringLiteral("^\\s*[0-9]+[.)]\\s+(.*)$"));
    static const QRegularExpression quoteRe(
        QStringLiteral("^\\s*>\\s?(.*)$"));
    static const QRegularExpression tblSepRe(
        QStringLiteral("^\\s*\\|?\\s*:?-{1,}:?\\s*"
                       "(?:\\|\\s*:?-{1,}:?\\s*)*\\|?\\s*$"));

    for (int i = 0; i < lines.size(); ++i) {
        const QString ln = lines.at(i);
        const bool isFence = ln.startsWith(QLatin1String("```"))
                             || ln.startsWith(QLatin1String("~~~"));

        // --- open display-math states -------------------------------
        if (!inCode && !inMathF && !inMathD && !inMathB && isFence
            && ln.mid(3).trimmed().startsWith(QLatin1String("math"))) {
            flushAll();
            inMathF = true;
            continue;
        }
        if (!inMathF && !inMathB && !inCode && ln.trimmed() == QLatin1String("$$")) {
            if (inMathD) {
                flushMath();
            } else {
                flushAll();
                inMathD = true;
            }
            continue;
        }
        if (!inMathF && !inMathD && !inCode
            && ln.trimmed() == QLatin1String("\\[")) {
            flushAll();
            inMathB = true;
            continue;
        }
        if (inMathB && ln.trimmed() == QLatin1String("\\]")) {
            flushMath();
            continue;
        }
        if (inMathF || inMathD || inMathB) {
            if (inMathF && isFence) {
                flushMath();
            } else {
                if (!mathBuf.isEmpty())
                    mathBuf += QLatin1Char('\n');
                mathBuf += ln;
            }
            continue;
        }

        // --- fenced code ---------------------------------------------
        if (inCode) {
            if (isFence) {
                flushCode();
            } else {
                if (!codeBuf.isEmpty())
                    codeBuf += QLatin1Char('\n');
                codeBuf += mdEscape(ln);
            }
            continue;
        }
        if (isFence) {
            flushAll();
            inCode = true;
            codeLabel = ln.mid(3).trimmed();
            codeLang = normalizeRunLang(codeLabel);
            continue;
        }

        QRegularExpressionMatch m = headingRe.match(ln);
        if (m.hasMatch()) {
            flushAll();
            const int level = m.captured(1).size();
            QString head = m.captured(2);
            static const QRegularExpression closingHashes(
                QStringLiteral("\\s+#+\\s*$"));
            head.remove(closingHashes);
            out += QStringLiteral("<h%1>%2</h%1>\n")
                       .arg(level)
                       .arg(mdInlinePass(head));
            continue;
        }

        if (hrRe.match(ln).hasMatch()) {
            flushAll();
            out += QStringLiteral("<hr>\n");
            continue;
        }

        m = quoteRe.match(ln);
        if (m.hasMatch() && ln.trimmed() != QLatin1String(">")) {
            flushPara();
            flushLists();
            quoteLines.append(m.captured(1));
            continue;
        }

        // Table block: header row, then separator row, then data rows
        if (tbl.isEmpty() && ln.contains(QLatin1Char('|'))
            && i + 1 < lines.size()
            && tblSepRe.match(lines.at(i + 1)).hasMatch()) {
            flushPara();
            flushLists();
            tbl.append(ln);
            ++i; // skip the separator row
            continue;
        }
        if (!tbl.isEmpty() && ln.contains(QLatin1Char('|'))) {
            flushPara();
            tbl.append(ln);
            continue;
        }
        flushTable();

        if (ln.trimmed().isEmpty()) {
            flushPara();
            flushLists();
            flushQuote();
            continue;
        }

        m = ulRe.match(ln);
        if (m.hasMatch()) {
            flushPara();
            flushQuote();
            if (!inUl) {
                flushLists();
                out += QStringLiteral("<ul>\n");
                inUl = true;
            }
            QString body = m.captured(1);
            if (body == QLatin1String("[ ]"))
                body = QStringLiteral("\u2610");
            else if (body.startsWith(QLatin1String("[ ] ")))
                body = QStringLiteral("\u2610 ") + body.mid(4);
            else if (body.compare(QLatin1String("[x]"), Qt::CaseInsensitive) == 0)
                body = QStringLiteral("\u2611");
            else if (body.startsWith(QLatin1String("[x] "), Qt::CaseInsensitive))
                body = QStringLiteral("\u2611 ") + body.mid(4);
            out += QStringLiteral("<li>%1</li>\n").arg(mdInlinePass(body));
            continue;
        }

        m = olRe.match(ln);
        if (m.hasMatch()) {
            flushPara();
            flushQuote();
            if (!inOl) {
                flushLists();
                out += QStringLiteral("<ol>\n");
                inOl = true;
            }
            out += QStringLiteral("<li>%1</li>\n").arg(mdInlinePass(m.captured(1)));
            continue;
        }

        if (!quoteLines.isEmpty())
            flushQuote();
        if (!para.isEmpty())
            para += QLatin1Char('\n');
        para += ln;
    }

    flushMath();
    flushCode();
    flushAll();
    return out;
}

static QString mdCss(bool dark)
{
    // Gruvbox paper tones inside the preview: bright headings, hard code
    // surfaces, muted blockquotes, gruvbox blue links.
    if (dark) {
        return QStringLiteral(
            "body { font-family: sans-serif; color: #ebdbb2; }\n"
            "h1 { font-size: 22pt; font-weight: bold; color: #fbf1c7; }\n"
            "h2 { font-size: 18pt; font-weight: bold; color: #fbf1c7; }\n"
            "h3 { font-size: 15pt; font-weight: bold; color: #ebdbb2; }\n"
            "h4 { font-size: 13pt; font-weight: bold; color: #ebdbb2; }\n"
            "h5 { font-size: 12pt; font-weight: bold; }\n"
            "h6 { font-size: 11pt; font-weight: bold; }\n"
            "code { font-family: monospace; background-color: #1d2021; }\n"
            "pre { font-family: monospace; background-color: #1d2021; }\n"
            "blockquote { color: #a89984; }\n"
            "a { color: #83a598; }\n"
            "hr { color: #3c3836; }\n"
            "th { background-color: #32302f; color: #fbf1c7; }\n");
    }
    return QStringLiteral(
        "body { font-family: sans-serif; color: #3c3836; }\n"
        "h1 { font-size: 22pt; font-weight: bold; color: #282828; }\n"
        "h2 { font-size: 18pt; font-weight: bold; color: #282828; }\n"
        "h3 { font-size: 15pt; font-weight: bold; color: #3c3836; }\n"
        "h4 { font-size: 13pt; font-weight: bold; }\n"
        "h5 { font-size: 12pt; font-weight: bold; }\n"
        "h6 { font-size: 11pt; font-weight: bold; }\n"
        "code { font-family: monospace; background-color: #eee5cf; }\n"
        "pre { font-family: monospace; background-color: #eee5cf; }\n"
        "blockquote { color: #7c6f64; }\n"
        "a { color: #076678; }\n"
        "hr { color: #d5c4a1; }\n"
        "th { background-color: #eee5cf; color: #282828; }\n");
}

static QString plainTextHtml(const QString &text)
{
    return QStringLiteral("<pre>%1</pre>").arg(mdEscape(text));
}

// Best-effort reader: strip the BOM, prefer UTF-8, otherwise Latin-1.
static QString readTextFileBest(const QString &path, QString *errOut = nullptr)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errOut)
            *errOut = file.errorString();
        return QString();
    }
    QByteArray raw = file.readAll();
    if (raw.startsWith("\xEF\xBB\xBF"))
        raw.remove(0, 3);
    QStringDecoder decoder(QStringConverter::Utf8,
                           QStringConverter::Flag::Stateless);
    QString text = decoder.decode(raw);
    if (decoder.hasError())
        return QString::fromLatin1(raw);
    return text;
}

static bool writePdfFromHtml(const QString &html, const QString &css,
                             const QString &outPath, QString *errOut)
{
    QPdfWriter pdf(outPath);
    QPageLayout lay(QPageSize(QPageSize::A4), QPageLayout::Portrait,
                    QMarginsF(18.0, 15.0, 18.0, 15.0),
                    QPageLayout::Millimeter);
    pdf.setPageLayout(lay);

    QTextDocument doc;
    if (!css.isEmpty())
        doc.setDefaultStyleSheet(css);
    doc.setHtml(html);
    doc.setPageSize(lay.paintRectPixels(pdf.resolution()).size());
    doc.print(&pdf);

    if (!QFile::exists(outPath)) {
        if (errOut)
            *errOut = QStringLiteral("Ziel-PDF wurde nicht erzeugt.");
        return false;
    }
    return true;
}

// Command-line converter: npplx --to-pdf <input> [output] [--plain]
static int convertToPdfCli(const QString &inPath, const QString &outPath,
                           bool plain)
{
    QString err;
    const QString text = readTextFileBest(inPath, &err);
    if (!err.isEmpty()) {
        QTextStream(stderr) << "Fehler beim Lesen: " << err << "\n";
        return 1;
    }

    const QString suffix = QFileInfo(inPath).suffix().toLower();
    const bool isMd = !plain
                      && (suffix == QLatin1String("md")
                          || suffix == QLatin1String("markdown")
                          || suffix == QLatin1String("mdown")
                          || suffix == QLatin1String("mkd"));

    QString html;
    QString css;
    if (isMd) {
        html = markdownToHtml(text);
        css = mdCss(false);
    } else {
        html = plainTextHtml(text);
        css = QStringLiteral("pre { font-family: monospace; font-size: 10pt; }");
    }

    if (writePdfFromHtml(html, css, outPath, &err)) {
        QTextStream(stdout) << "PDF erstellt: " << outPath << "\n";
        return 0;
    }
    QTextStream(stderr) << "Fehler beim PDF-Export: " << err << "\n";
    return 1;
}

// ---------------------------------------------------------------------------
// Main window (counterpart of the buffer/dock management of the original)
// ---------------------------------------------------------------------------

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr)
        : QMainWindow(parent)
        , settings(QStringLiteral("NppLX"), QStringLiteral("NppLX"))
    {
        fontSize = settings.value(QStringLiteral("view/fontsize"),
                                 DefaultFontSize).toInt();
        font.setPointSize(fontSize);
        flagLineno = settings.value(QStringLiteral("view/lineno"), true).toBool();
        flagWrap   = settings.value(QStringLiteral("view/wrap"), false).toBool();
        flagWs     = settings.value(QStringLiteral("view/whitespace"), false).toBool();
        flagGuides = settings.value(QStringLiteral("view/indentguides"), false).toBool();
        dark       = settings.value(QStringLiteral("view/theme"),
                                    QStringLiteral("light")).toString()
                        == QStringLiteral("dark");

        setWindowTitle(QStringLiteral("NppLX"));
        resize(1000, 680);

        tabs = new QTabWidget(this);
        tabs->setTabsClosable(true);
        tabs->setMovable(true);
        tabs->setDocumentMode(true);
        connect(tabs, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
        connect(tabs, &QTabWidget::currentChanged, this, [this](int){ onTabChanged(); });
        setCentralWidget(tabs);

        buildDocks();
        buildMenus();

        labelPos  = new QLabel(this);
        labelLen  = new QLabel(this);
        labelInfo = new QLabel(this);
        for (QLabel *label : {labelPos, labelLen, labelInfo}) {
            label->setContentsMargins(8, 0, 8, 0);
            statusBar()->addPermanentWidget(label);
        }

        font.setFamily(resolveMonoFamily());
        setTheme(dark);

        const QByteArray geo = settings.value(
            QStringLiteral("window/geometry")).toByteArray();
        if (!geo.isEmpty())
            restoreGeometry(geo);

        restoreSession();
        if (tabs->count() == 0)
            newDocument();
        else
            onTabChanged();

        auto *nextTab = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Tab")), this);
        connect(nextTab, &QShortcut::activated, this, [this]{ cycleTab(1); });
        auto *prevTab = new QShortcut(
            QKeySequence(QStringLiteral("Ctrl+Shift+Tab")), this);
        connect(prevTab, &QShortcut::activated, this, [this]{ cycleTab(-1); });
    }

    QSettings settings;
    QFont font;
    bool dark = false;
    bool flagLineno = true;
    bool flagWrap = false;
    bool flagWs = false;
    bool flagGuides = false;
    int fontSize = DefaultFontSize;
    int newCounter = 0;

    QTabWidget *tabs = nullptr;
    QLabel *labelPos = nullptr;
    QLabel *labelLen = nullptr;
    QLabel *labelInfo = nullptr;
    QVector<Editor *> editors;
    QMenu *recentMenu = nullptr;
    FindReplaceDialog *findDialog = nullptr;
    QDockWidget *dockMd = nullptr;
    QTextBrowser *mdView = nullptr;
    QTimer *mdTimer = nullptr;
    QDockWidget *dockResults = nullptr;
    QTreeWidget *resultsTree = nullptr;
    QLabel *resultsHeader = nullptr;
    QAction *actMdDock = nullptr;
    QList<RunBlock> mdRunBlocks;
    QHash<QString, QString> runResults;
    QString runKeyRunning;
    RunJob *runJob = nullptr;
    QList<QAction *> themeMenuActs;
    QAction *actThemeToggle = nullptr;

    // ----------------------------------------------------------------- Access

    Editor *currentEditor() const
    {
        return static_cast<Editor *>(tabs->currentWidget());
    }

    // --------------------------------------------------------- Editor setup

    void configureEditor(Editor *ed)
    {
        ed->setFont(font);
        ed->setMarginLineNumbers(0, flagLineno);
        ed->setMarginWidth(0, ed->fontMetrics().horizontalAdvance(
                                 QStringLiteral("000000")) + 12);
        ed->setMarginWidth(1, 18);
        ed->setMarginWidth(2, 16);
        ed->setWrapMode(flagWrap ? QsciScintilla::WrapWord
                                 : QsciScintilla::WrapNone);
        ed->SendScintilla(QsciScintillaBase::SCI_SETVIEWWS, flagWs ? 1 : 0);
        ed->SendScintilla(QsciScintillaBase::SCI_SETINDENTATIONGUIDES,
                          flagGuides ? 3 : 0);
        applyColors(ed);

        connect(ed, &QsciScintilla::modificationChanged, this,
                [this](bool){ refreshTabLabels(); updateWindowTitle(); });
        connect(ed, &QsciScintilla::cursorPositionChanged, this,
                [this](int, int){ updateStatus(); });
        connect(ed, &QsciScintilla::textChanged, this, [this]{
            updateStatus();
            if (mdTimer)
                mdTimer->start();
        });
        connect(ed, &QsciScintilla::marginClicked, this,
                [this, ed](int margin, int line, Qt::KeyboardModifiers) {
                    if (margin == 1) {
                        ed->SendScintilla(QsciScintillaBase::SCI_TOGGLEFOLD,
                                          static_cast<long>(line));
                    } else if (margin == 2) {
                        toggleBookmarkAt(ed, line);
                    }
                });
    }

    void applyColors(Editor *ed)
    {
        const QColor paper   = dark ? QColor("#282828") : QColor("#fdfaf2");
        const QColor def     = dark ? QColor("#ebdbb2") : QColor("#3c3836");
        const QColor caret   = dark ? QColor("#fabd2f") : QColor("#b57614");
        const QColor caretln = dark ? QColor("#32302f") : QColor("#f2ecdc");
        const QColor selBg   = dark ? QColor("#504945") : QColor("#d5c4a1");
        const QColor selFg   = dark ? QColor("#fbf1c7") : QColor("#282828");
        const QColor edge    = dark ? QColor("#3c3836") : QColor("#d5c4a1");
        const QColor foldFg  = dark ? QColor("#665c54") : QColor("#a89984");
        const QColor foldBg  = dark ? QColor("#1d2021") : QColor("#f2ecdc");

        ed->setPaper(paper);
        ed->setColor(def);
        // Gutter: margin surfaces follow the chrome, line numbers muted.
        const QColor gutterBg = dark ? QColor("#1d2021") : QColor("#f2ecdc");
        for (int m = 0; m < 3; ++m)
            ed->setMarginBackgroundColor(m, gutterBg);
        ed->SendScintilla(QsciScintillaBase::SCI_STYLESETFORE, 33,
                          static_cast<unsigned long>(((foldFg.blue() << 16) | (foldFg.green() << 8) | foldFg.red())));
        ed->SendScintilla(QsciScintillaBase::SCI_STYLESETBACK, 33,
                          static_cast<unsigned long>(((gutterBg.blue() << 16) | (gutterBg.green() << 8) | gutterBg.red())));
        ed->setCaretForegroundColor(caret);
        ed->setCaretLineBackgroundColor(caretln);
        ed->setCaretLineVisible(true);
        ed->setSelectionBackgroundColor(selBg);
        ed->setSelectionForegroundColor(selFg);
        ed->setEdgeColor(edge);
        ed->setFoldMarginColors(foldFg, foldBg);
        applyLexer(ed, ed->lang);
    }

    void applyLexer(Editor *ed, const QString &lang)
    {
        ed->lang = lang;
        QsciLexer *old = ed->lexer();
        QsciLexer *lex = lang.isEmpty() ? nullptr : makeLexer(lang, ed);
        ed->setLexer(lex);
        if (lex) {
            lex->setFont(font);
            // Base style follows the editor paper; accent roles come from
            // roleColor for both themes (gruvbox accents, darkened for
            // light mode).
            lex->setColor(dark ? QColor("#ebdbb2") : QColor("#3c3836"), -1);
            lex->setPaper(dark ? QColor("#282828") : QColor("#fdfaf2"), -1);
            const auto it = DarkStyles.find(lang.toStdString());
            if (it != DarkStyles.end()) {
                for (const auto &entry : it->second)
                    lex->setColor(roleColor(entry.second, dark), entry.first);
            }
        }
        if (old && old != lex)
            old->deleteLater();
        updateStatus();
    }

    // ----------------------------------------------- Document management

    void addEditor(Editor *ed, const QString &title)
    {
        editors.append(ed);
        const int index = tabs->addTab(ed, title);
        tabs->setCurrentIndex(index);
        configureEditor(ed);
        ed->detectEol(ed->text());
    }

    void newDocument()
    {
        ++newCounter;
        auto *ed = new Editor(this);
        ed->setText(QString());
        ed->setModified(false);
        addEditor(ed, QStringLiteral("Neu %1").arg(newCounter));
        refreshTabLabels();
        updateWindowTitle();
        ed->setFocus();
    }

    void openFilesDialog()
    {
        const QString start = settings.value(QStringLiteral("state/lastdir"),
                                             QDir::homePath()).toString();
        const QStringList paths = QFileDialog::getOpenFileNames(
            this, QStringLiteral("Datei(en) öffnen"), start);
        for (const QString &path : paths)
            openFile(path);
        if (!paths.isEmpty())
            settings.setValue(QStringLiteral("state/lastdir"),
                              QFileInfo(paths.first()).absolutePath());
    }

    void openFile(QString path)
    {
        if (path.isEmpty())
            return;
        path = QFileInfo(path).absoluteFilePath();
        for (Editor *ed : editors) {
            if (ed->filepath == path) {          // already open: activate it
                tabs->setCurrentWidget(ed);
                return;
            }
        }

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            statusBar()->showMessage(
                QStringLiteral("Datei nicht lesbar: %1").arg(path), 5000);
            return;
        }
        QByteArray raw = file.readAll();
        file.close();
        if (raw.startsWith("\xEF\xBB\xBF"))
            raw.remove(0, 3);

        QStringDecoder decoder(QStringConverter::Utf8,
                               QStringConverter::Flag::Stateless);
        QString text = decoder.decode(raw);

        auto *ed = new Editor(this);
        if (decoder.hasError()) {
            text = QString::fromLatin1(raw);
            ed->encoding = QStringLiteral("ansi");
        } else {
            ed->encoding = QStringLiteral("utf-8");
        }
        ed->setText(text);
        ed->setModified(false);
        ed->filepath = path;
        ed->lang = langForPath(path);
        addEditor(ed, ed->baseName());
        refreshTabLabels();
        pushRecent(path);
        updateStatus();
    }

    void saveCurrent()
    {
        Editor *ed = currentEditor();
        if (ed)
            saveEditor(ed);
    }

    void saveCurrentAs()
    {
        Editor *ed = currentEditor();
        if (!ed)
            return;
        const QString suggested = ed->filepath.isEmpty()
            ? settings.value(QStringLiteral("state/lastdir"),
                             QDir::homePath()).toString()
                + QLatin1Char('/') + ed->baseName()
            : ed->filepath;
        const QString path = QFileDialog::getSaveFileName(
            this, QStringLiteral("Speichern unter"), suggested);
        if (path.isEmpty())
            return;
        ed->filepath = QFileInfo(path).absoluteFilePath();
        ed->lang = langForPath(path);
        applyLexer(ed, ed->lang);
        settings.setValue(QStringLiteral("state/lastdir"),
                          QFileInfo(path).absolutePath());
        writeEditor(ed);
    }

    void saveEditor(Editor *ed)
    {
        if (ed->filepath.isEmpty())
            saveCurrentAs();
        else
            writeEditor(ed);
    }

    void saveAll()
    {
        for (Editor *ed : editors)
            if (ed->isModified())
                saveEditor(ed);
    }

    bool writeEditor(Editor *ed)
    {
        const QByteArray data =
            ed->encoding == QStringLiteral("ansi")
                ? ed->text().toLatin1()
                : ed->text().toUtf8();
        QFile file(ed->filepath);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, QStringLiteral("Fehler beim Speichern"),
                                   file.errorString());
            return false;
        }
        file.write(data);
        file.close();
        ed->setModified(false);
        pushRecent(ed->filepath);
        refreshTabLabels();
        updateWindowTitle();
        statusBar()->showMessage(
            QStringLiteral("Gespeichert: %1").arg(ed->filepath), 4000);
        return true;
    }

    bool maybeSave(Editor *ed)
    {
        if (!ed->isModified())
            return true;
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this, QStringLiteral("Ungespeicherte Änderungen"),
            QStringLiteral("Möchten Sie die Änderungen an \"%1\" speichern?")
                .arg(ed->displayTitle()),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (answer == QMessageBox::Save) {
            saveEditor(ed);
            return !ed->isModified();
        }
        return answer != QMessageBox::Cancel;
    }

    void closeTab(int index)
    {
        if (index < 0)
            return;
        auto *ed = static_cast<Editor *>(tabs->widget(index));
        if (!ed)
            return;
        if (!maybeSave(ed))
            return;
        editors.removeAll(ed);
        tabs->removeTab(index);
        ed->deleteLater();
        if (tabs->count() == 0)
            newDocument();
        else
            onTabChanged();
    }

    void closeAll()
    {
        while (tabs->count() > 0)
            closeTab(tabs->count() - 1);
    }

    // ------------------------------------------------------------- Search

    void showFindDialog()
    {
        if (!findDialog)
            findDialog = new FindReplaceDialog(this);
        Editor *ed = currentEditor();
        if (ed && ed->hasSelectedText())
            findDialog->editFind->setText(ed->selectedText());
        findDialog->show();
        findDialog->raise();
        findDialog->activateWindow();
        findDialog->editFind->setFocus();
        findDialog->editFind->selectAll();
    }

    void findStep(bool backward)
    {
        if (!findDialog) {
            showFindDialog();
            return;
        }
        findDialog->findFrom(backward);
    }

    void gotoLine()
    {
        Editor *ed = currentEditor();
        if (!ed)
            return;
        int line = 0, index = 0;
        ed->getCursorPosition(&line, &index);
        bool ok = false;
        const int target = QInputDialog::getInt(
            this, QStringLiteral("Gehe zu Zeile"),
            QStringLiteral("Zeilennummer (1 - %1):").arg(ed->lines()),
            line + 1, 1, qMax(1, ed->lines()), 1, &ok);
        if (!ok)
            return;
        ed->setCursorPosition(target - 1, 0);
        ed->ensureLineVisible(target - 1);
    }

    // ---------------------------------------------------- View options

    void zoom(int delta, bool reset = false)
    {
        fontSize = reset ? DefaultFontSize
                         : qBound(MinFontSize, fontSize + delta, MaxFontSize);
        font.setPointSize(fontSize);
        for (Editor *ed : editors) {
            ed->setFont(font);
            if (QsciLexer *lex = ed->lexer())
                lex->setFont(font);
            ed->setMarginWidth(0, ed->fontMetrics().horizontalAdvance(
                                     QStringLiteral("000000")) + 12);
        }
    }

    void setTheme(bool darkTheme)
    {
        dark = darkTheme;
        setPreviewDark(darkTheme);
        const QPalette pal = studioPalette(darkTheme);
        QApplication::setPalette(pal);
        for (QWidget *w : QApplication::allWidgets())
            w->setPalette(pal);
        qApp->setStyleSheet(appStyleSheet(darkTheme));
        for (QAction *a : themeMenuActs)
            a->setChecked(a->property("themeDark").toBool() == darkTheme);
        if (actThemeToggle) {
            const QSignalBlocker guard(actThemeToggle);
            actThemeToggle->setChecked(darkTheme);
            actThemeToggle->setText(darkTheme
                                        ? QStringLiteral("Hell")
                                        : QStringLiteral("Dunkel"));
        }
        for (Editor *ed : editors)
            applyColors(ed);
        // Palette/style changes can override the QScintilla margin style;
        // re-assert the gutter colours on the next event-loop turn.
        QTimer::singleShot(0, this, [this]{
            for (Editor *ed : editors) {
                const QColor gutterBg = dark ? QColor("#1d2021")
                                             : QColor("#f2ecdc");
                const QColor foldFg = dark ? QColor("#665c54")
                                            : QColor("#a89984");
                for (int m = 0; m < 3; ++m)
                    ed->setMarginBackgroundColor(m, gutterBg);
                ed->SendScintilla(QsciScintillaBase::SCI_STYLESETFORE, 33,
                                  static_cast<unsigned long>(
                                      ((foldFg.blue() << 16) |
                                       (foldFg.green() << 8) |
                                       foldFg.red())));
                ed->SendScintilla(QsciScintillaBase::SCI_STYLESETBACK, 33,
                                  static_cast<unsigned long>(
                                      ((gutterBg.blue() << 16) |
                                       (gutterBg.green() << 8) |
                                       gutterBg.red())));
            }
            // Scintilla's scroll bars do not re-polish with the app
            // sheet on theme changes; style them per editor.
            const QString sbTrough = dark
                ? QStringLiteral("#1d2021") : QStringLiteral("#f2ecdc");
            const QString sbThumb = dark
                ? QStringLiteral("#504945") : QStringLiteral("#c9b68e");
            const QString sbHover = dark
                ? QStringLiteral("#665c54") : QStringLiteral("#a89984");
            const QString sbSheet = QStringLiteral(
                "QScrollBar:vertical{background:%1;width:10px;margin:2px;border:none;}"
                "QScrollBar::handle:vertical{background:%2;border-radius:4px;"
                "min-height:24px;}"
                "QScrollBar::handle:vertical:hover{background:%3;}"
                "QScrollBar:horizontal{background:%1;height:10px;margin:2px;border:none;}"
                "QScrollBar::handle:horizontal{background:%2;border-radius:4px;"
                "min-width:24px;}"
                "QScrollBar::handle:horizontal:hover{background:%3;}"
                "QScrollBar::add-line,QScrollBar::sub-line{height:0px;width:0px;}"
                "QScrollBar::add-page,QScrollBar::sub-page{background:transparent;}")
                .arg(sbTrough, sbThumb, sbHover);
            for (Editor *ed : editors) {
                ed->verticalScrollBar()->setStyleSheet(sbSheet);
                ed->horizontalScrollBar()->setStyleSheet(sbSheet);
            }
        });
        if (mdView && dockMd && dockMd->isVisible())
            renderMd();
        updateStatus();
    }

    // --------------------------------------------- Recently opened

    void pushRecent(const QString &path)
    {
        QStringList recent = settings.value(
            QStringLiteral("recent/files")).toStringList();
        recent.removeAll(path);
        recent.prepend(path);
        while (recent.size() > MaxRecent)
            recent.removeLast();
        settings.setValue(QStringLiteral("recent/files"), recent);
        rebuildRecentMenu();
    }

    void rebuildRecentMenu()
    {
        recentMenu->clear();
        const QStringList recent = settings.value(
            QStringLiteral("recent/files")).toStringList();
        for (const QString &path : recent) {
            QAction *act = recentMenu->addAction(path);
            connect(act, &QAction::triggered, this,
                    [this, path]{ openFile(path); });
        }
        if (recentMenu->isEmpty()) {
            QAction *empty = recentMenu->addAction(QStringLiteral("(leer)"));
            empty->setEnabled(false);
        }
    }

    // -------------------------------------------------- Display updates

    void refreshTabLabels()
    {
        for (Editor *ed : editors) {
            const int index = tabs->indexOf(ed);
            if (index >= 0) {
                tabs->setTabText(index, ed->displayTitle());
                tabs->setTabToolTip(index,
                    ed->filepath.isEmpty()
                        ? QStringLiteral("Unbenanntes Dokument")
                        : ed->filepath);
            }
        }
    }

    void onTabChanged()
    {
        refreshTabLabels();
        updateWindowTitle();
        updateStatus();
        syncMdDock();
    }

    void updateWindowTitle()
    {
        Editor *ed = currentEditor();
        if (!ed) {
            setWindowTitle(QStringLiteral("NppLX"));
            setWindowModified(false);
            return;
        }
        setWindowTitle(QStringLiteral("%1[*] - NppLX").arg(ed->baseName()));
        setWindowModified(ed->isModified());
    }

    void updateStatus()
    {
        Editor *ed = currentEditor();
        if (!ed) {
            labelPos->setText(QLatin1String("-"));
            labelLen->setText(QLatin1String("-"));
            labelInfo->setText(QLatin1String("-"));
            return;
        }
        int line = 0, index = 0;
        ed->getCursorPosition(&line, &index);
        labelPos->setText(QStringLiteral("Zeile %1, Spalte %2")
                          .arg(line + 1).arg(index + 1));
        labelLen->setText(QStringLiteral("Länge: %1").arg(ed->text().length()));
        labelInfo->setText(QStringLiteral("%1 | %2 | %3")
            .arg(ed->encoding == QStringLiteral("ansi")
                     ? QStringLiteral("ANSI") : QStringLiteral("UTF-8"),
                 Editor::eolName(ed->eolMode()),
                 ed->lang.isEmpty() ? QStringLiteral("Normaltext")
                                    : ed->lang));
    }

    void cycleTab(int step)
    {
        if (tabs->count() < 2)
            return;
        tabs->setCurrentIndex((tabs->currentIndex() + step + tabs->count())
                              % tabs->count());
    }

    // ---------------------------------------------------------------- Session

    void restoreSession()
    {
        rebuildRecentMenu();
        const QStringList files = settings.value(
            QStringLiteral("session/files")).toStringList();
        for (const QString &path : files) {
            if (QFile::exists(path))
                openFile(path);
        }
        const QStringList args = qApp->arguments();
        for (int i = 1; i < args.size(); ++i) {
            if (QFile::exists(args.at(i)))
                openFile(QFileInfo(args.at(i)).absoluteFilePath());
        }
    }

    void saveSession()
    {
        settings.setValue(QStringLiteral("view/lineno"), flagLineno);
        settings.setValue(QStringLiteral("view/wrap"), flagWrap);
        settings.setValue(QStringLiteral("view/whitespace"), flagWs);
        settings.setValue(QStringLiteral("view/indentguides"), flagGuides);
        settings.setValue(QStringLiteral("view/theme"),
                          dark ? QStringLiteral("dark") : QStringLiteral("light"));
        settings.setValue(QStringLiteral("view/fontsize"), fontSize);
        settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
        QStringList open;
        for (Editor *ed : editors)
            if (!ed->filepath.isEmpty())
                open.append(ed->filepath);
        settings.setValue(QStringLiteral("session/files"), open);
    }

protected:
    void closeEvent(QCloseEvent *event) override
    {
        for (Editor *ed : editors) {
            if (!maybeSave(ed)) {
                event->ignore();
                return;
            }
        }
        saveSession();
        event->accept();
    }

    // ------------------------------------------------------- Docks & Markdown

    void buildDocks()
    {
        mdTimer = new QTimer(this);
        mdTimer->setSingleShot(true);
        mdTimer->setInterval(400);
        connect(mdTimer, &QTimer::timeout, this, [this]{ renderMd(); });

        auto *dockBody = new QWidget(this);
        auto *vbox = new QVBoxLayout(dockBody);
        vbox->setContentsMargins(0, 0, 0, 0);
        vbox->setSpacing(0);

        auto *headBar = new QWidget(dockBody);
        auto *hbox = new QHBoxLayout(headBar);
        hbox->setContentsMargins(8, 4, 8, 4);
        hbox->addWidget(new QLabel(QStringLiteral("Markdown-Vorschau"), headBar), 1);
        auto *btnPdf = new QPushButton(QStringLiteral("Als PDF exportieren"), headBar);
        connect(btnPdf, &QPushButton::clicked, this, [this]{ exportPdf(); });
        hbox->addWidget(btnPdf);
        vbox->addWidget(headBar);

        mdView = new QTextBrowser(dockBody);
        mdView->setOpenExternalLinks(false);
        connect(mdView, &QTextBrowser::anchorClicked, this,
                &MainWindow::onMdAnchor);
        vbox->addWidget(mdView);

        dockMd = new QDockWidget(QStringLiteral("Markdown-Ansicht"), this);
        dockMd->setWidget(dockBody);
        dockMd->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea
                                | Qt::BottomDockWidgetArea);
        dockMd->setFeatures(QDockWidget::DockWidgetClosable
                            | QDockWidget::DockWidgetMovable
                            | QDockWidget::DockWidgetFloatable);
        dockMd->hide();
        addDockWidget(Qt::RightDockWidgetArea, dockMd);
        connect(dockMd, &QDockWidget::visibilityChanged, this, [this](bool on){
            if (actMdDock) {
                const QSignalBlocker guard(actMdDock);
                actMdDock->setChecked(on);
            }
        });

        auto *resBody = new QWidget(this);
        auto *rvbox = new QVBoxLayout(resBody);
        rvbox->setContentsMargins(0, 0, 0, 0);
        rvbox->setSpacing(0);
        resultsHeader = new QLabel(QStringLiteral("Ergebnisse"), resBody);
        resultsHeader->setContentsMargins(8, 4, 8, 4);
        rvbox->addWidget(resultsHeader);
        resultsTree = new QTreeWidget(resBody);
        resultsTree->setColumnCount(2);
        resultsTree->setHeaderLabels({QStringLiteral("Fundstelle"),
                                      QStringLiteral("Text")});
        resultsTree->setRootIsDecorated(true);
        resultsTree->setAlternatingRowColors(true);
        rvbox->addWidget(resultsTree);

        auto jump = [this](QTreeWidgetItem *item) {
            if (!item)
                return;
            const int lineNo = item->data(0, Qt::UserRole + 1).toInt();
            QString path = item->data(0, Qt::UserRole).toString();
            if (item->parent()) {
                const QString parentPath =
                    item->parent()->data(0, Qt::UserRole).toString();
                if (!parentPath.isEmpty())
                    path = parentPath;
            }
            if (!path.isEmpty())
                openFile(path);
            Editor *ed = currentEditor();
            if (ed && lineNo >= 0) {
                const int target = qMin(lineNo, ed->lines() - 1);
                ed->setCursorPosition(target, 0);
                ed->ensureLineVisible(target);
                ed->setFocus();
            }
        };
        connect(resultsTree, &QTreeWidget::itemActivated, this,
                [jump](QTreeWidgetItem *item, int){ jump(item); });
        connect(resultsTree, &QTreeWidget::itemClicked, this,
                [jump](QTreeWidgetItem *item, int){ jump(item); });

        dockResults = new QDockWidget(QStringLiteral("Suchergebnisse"), this);
        dockResults->setWidget(resBody);
        dockResults->setFeatures(QDockWidget::DockWidgetClosable
                                 | QDockWidget::DockWidgetMovable
                                 | QDockWidget::DockWidgetFloatable);
        dockResults->hide();
        addDockWidget(Qt::BottomDockWidgetArea, dockResults);
    }

    void renderMd()
    {
        if (!mdView || !dockMd->isVisible())
            return;
        Editor *ed = currentEditor();
        if (!ed) {
            mdView->clear();
            return;
        }
        if (ed->lang != QLatin1String("Markdown")) {
            mdView->document()->setDefaultStyleSheet(mdCss(dark));
            mdView->setHtml(QStringLiteral(
                "<p>Die Markdown-Vorschau rendert das aktive Markdown-"
                "Dokument live mit.</p>"
                "<p>Markdown erkennen: Datei mit Endung <code>.md</code> "
                "oeffnen oder Sprache per Ansicht setzen.</p>"));
            return;
        }
        mdView->document()->setDefaultStyleSheet(mdCss(dark));
        mdRunBlocks.clear();
        mdView->setHtml(markdownToHtml(ed->text(), &mdRunBlocks,
                                       &runResults, runKeyRunning));
    }

    void syncMdDock()
    {
        if (!dockMd)
            return;
        Editor *ed = currentEditor();
        const bool isMd = ed && ed->lang == QLatin1String("Markdown");
        if (isMd && !dockMd->isVisible()) {
            dockMd->setVisible(true);
            if (actMdDock) {
                const QSignalBlocker guard(actMdDock);
                actMdDock->setChecked(true);
            }
        }
        renderMd();
    }

    // --------------------------------------------- Code-block execution
    // Executes ```c / ```cpp / ```python / ```sql blocks shown in the
    // Markdown preview. Security posture: explicit user click only,
    // absolute program paths from the settings (no PATH lookup, no shell),
    // processes run with a 10 s kill timer and a 100 KB output cap, all
    // temporary files live in a private QTemporaryDir (0700) that is
    // removed afterwards.

    void onMdAnchor(const QUrl &url)
    {
        const QString s = url.toString();
        static const QString scheme = QStringLiteral("npplxrun:");
        if (s.startsWith(scheme)) {
            bool ok = false;
            const int id = s.mid(scheme.size()).toInt(&ok);
            if (ok)
                runCodeBlock(id);
            return;
        }
        if (url.scheme() == QLatin1String("http")
            || url.scheme() == QLatin1String("https"))
            QDesktopServices::openUrl(url);
    }

    void showExecDialog()
    {
        QDialog dlg(this);
        dlg.setWindowTitle(
            QStringLiteral("Werkzeuge für Code-Ausführung"));
        auto *grid = new QGridLayout(&dlg);
        const QVector<QPair<QString, QPair<QString, QString>>> rows = {
            {QStringLiteral("exec/gcc"),
             {QStringLiteral("C-Compiler (gcc):"), QStringLiteral("c")}},
            {QStringLiteral("exec/gxx"),
             {QStringLiteral("C++-Compiler (g++):"), QStringLiteral("cpp")}},
            {QStringLiteral("exec/python"),
             {QStringLiteral("Python-Interpreter:"), QStringLiteral("python")}},
            {QStringLiteral("exec/sqlite"),
             {QStringLiteral("SQLite-Kommandozeile:"), QStringLiteral("sql")}},
        };
        QVector<QLineEdit *> edits;
        for (int r = 0; r < rows.size(); ++r) {
            grid->addWidget(new QLabel(rows.at(r).second.first, &dlg), r, 0);
            auto *le = new QLineEdit(&dlg);
            le->setText(settings.value(rows.at(r).first,
                                       defaultExecutor(rows.at(r).second.second))
                            .toString());
            edits.append(le);
            grid->addWidget(le, r, 1);
            auto *btn = new QPushButton(QStringLiteral("…"), &dlg);
            const QString cfgKey = rows.at(r).first;
            const QString lang = rows.at(r).second.second;
            connect(btn, &QPushButton::clicked, this,
                    [this, &dlg, le, cfgKey, lang] {
                        const QString cur = le->text().isEmpty()
                                                ? defaultExecutor(lang)
                                                : le->text();
                        const QString sel = QFileDialog::getOpenFileName(
                            &dlg, QStringLiteral("Werkzeug auswählen"), cur);
                        if (!sel.isEmpty())
                            le->setText(sel);
                    });
            grid->addWidget(btn, r, 2);
        }
        auto *box = new QDialogButtonBox(QDialogButtonBox::Ok
                                         | QDialogButtonBox::Cancel, &dlg);
        connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        grid->addWidget(box, rows.size(), 0, 1, 3);
        dlg.resize(560, 220);
        if (dlg.exec() == QDialog::Accepted) {
            for (int r = 0; r < rows.size(); ++r)
                settings.setValue(rows.at(r).first,
                                  edits.at(r)->text().trimmed());
            statusBar()->showMessage(
                QStringLiteral("Werkzeuge gespeichert."), 4000);
        }
    }

    void appendRunOutput(const QString &chunk)
    {
        if (!runJob)
            return;
        if (runJob->capped)
            return;
        runJob->output += chunk;
        if (runJob->output.size() > 100000) {
            runJob->output = runJob->output.left(100000);
            runJob->output += QStringLiteral("\n(Ausgabe gekürzt)");
            runJob->capped = true;
        }
    }

    void cleanupRunJob()
    {
        if (!runJob)
            return;
        if (runJob->killer) {
            runJob->killer->stop();
            runJob->killer->deleteLater();
        }
        if (runJob->proc)
            runJob->proc->deleteLater();
        delete runJob->dir; // removes the whole temporary tree
        delete runJob;
        runJob = nullptr;
    }

    void finishRun(int exitCode, const QString &phase)
    {
        if (!runJob)
            return;
        const qint64 ms = runJob->clock.elapsed();
        const QString res = formatRunResult(exitCode, runJob->output, ms,
                                            runJob->timedOut, phase);
        const QString key = runJob->key;
        runKeyRunning.clear();
        cleanupRunJob();
        runResults.insert(key, res);
        renderMd();
        statusBar()->showMessage(
            QStringLiteral("Ausführung beendet."), 4000);
    }

    void onRunPhaseFinished(int exitCode)
    {
        if (!runJob || !runJob->proc)
            return;
        appendRunOutput(QString::fromUtf8(runJob->proc->readAllStandardOutput()));
        if (runJob->phase == 0) { // compile step finished
            if (exitCode == 0 && QFile::exists(runJob->binPath)) {
                runJob->phase = 1;
                runJob->clock.restart();
                runJob->proc->start(runJob->binPath, QStringList());
                return;
            }
            finishRun(exitCode, QStringLiteral("Compiler"));
            return;
        }
        finishRun(exitCode, QString());
    }

    void runCodeBlock(int idx)
    {
        if (idx < 0 || idx >= mdRunBlocks.size())
            return;
        if (runJob) {
            statusBar()->showMessage(
                QStringLiteral("Es läuft bereits eine Ausführung."), 4000);
            return;
        }
        const RunBlock rb = mdRunBlocks.at(idx);
        const QString key = runBlockKey(rb.lang, rb.code);
        QString cfgKey = QStringLiteral("exec/gcc");
        if (rb.lang == QLatin1String("cpp"))
            cfgKey = QStringLiteral("exec/gxx");
        else if (rb.lang == QLatin1String("python"))
            cfgKey = QStringLiteral("exec/python");
        else if (rb.lang == QLatin1String("sql"))
            cfgKey = QStringLiteral("exec/sqlite");
        QString prog = settings.value(cfgKey,
                                      defaultExecutor(rb.lang)).toString();
        if (prog.isEmpty())
            prog = defaultExecutor(rb.lang);
        if (!QFileInfo::exists(prog)) {
            QMessageBox::warning(
                this, QStringLiteral("Code-Ausführung"),
                QStringLiteral("Werkzeug nicht gefunden:\n%1\n\nBitte "
                               "unter Extras → Werkzeuge für "
                               "Code-Ausführung einen gültigen "
                               "Programmpfad eintragen.").arg(prog));
            showExecDialog();
            return;
        }

        runJob = new RunJob;
        runJob->dir = new QTemporaryDir;
        const QString base = runJob->dir->path();
        QString scriptPath;
        if (rb.lang == QLatin1String("c"))
            scriptPath = base + QStringLiteral("/main.c");
        else if (rb.lang == QLatin1String("cpp"))
            scriptPath = base + QStringLiteral("/main.cpp");
        else if (rb.lang == QLatin1String("python"))
            scriptPath = base + QStringLiteral("/script.py");
        else
            scriptPath = base + QStringLiteral("/script.sql");
        {
            QFile f(scriptPath);
            if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                QMessageBox::critical(
                    this, QStringLiteral("Code-Ausführung"),
                    QStringLiteral("Temporäre Datei konnte nicht angelegt "
                                   "werden."));
                cleanupRunJob();
                return;
            }
            f.write(mdUnescape(rb.code).toUtf8());
            f.close();
            QFile::setPermissions(scriptPath,
                                   QFile::ReadOwner | QFile::WriteOwner);
        }
        Editor *ed = currentEditor();
        const QString workDir = (ed && !ed->filepath.isEmpty())
                                    ? QFileInfo(ed->filepath).absolutePath()
                                    : base;

        runJob->lang = rb.lang;
        runJob->key = key;
        runJob->binPath = base + QStringLiteral("/npplx-run.out");
        runJob->phase = (rb.lang == QLatin1String("c")
                         || rb.lang == QLatin1String("cpp")) ? 0 : 2;
        runJob->proc = new QProcess(this);
        runJob->proc->setProcessChannelMode(QProcess::MergedChannels);
        runJob->proc->setWorkingDirectory(workDir);
        connect(runJob->proc, &QProcess::readyReadStandardOutput, this, [this] {
            if (runJob && runJob->proc)
                appendRunOutput(
                    QString::fromUtf8(runJob->proc->readAllStandardOutput()));
        });
        connect(runJob->proc, &QProcess::finished, this,
                [this](int exitCode, QProcess::ExitStatus st) {
                    Q_UNUSED(st);
                    onRunPhaseFinished(exitCode);
                });
        connect(runJob->proc, &QProcess::errorOccurred, this,
                [this](QProcess::ProcessError err) {
                    if (!runJob)
                        return;
                    if (err == QProcess::FailedToStart) {
                        appendRunOutput(QStringLiteral(
                            "Start fehlgeschlagen: Programm konnte nicht "
                            "gestartet werden.\n"));
                        runJob->proc->deleteLater();
                        runJob->proc = nullptr;
                        finishRun(-1, QString());
                    }
                });

        runJob->killer = new QTimer(this);
        connect(runJob->killer, &QTimer::timeout, this, [this] {
            if (runJob && runJob->proc) {
                runJob->timedOut = true;
                runJob->proc->kill();
                appendRunOutput(QStringLiteral(
                    "\n(Ausführung nach 10 s abgebrochen)\n"));
                // finished() will still fire; if kill is ineffective the
                // handler keeps the job alive until it exits.
            }
        });
        runJob->killer->setSingleShot(true);
        runJob->killer->start(10000);

        runKeyRunning = key;
        runJob->clock.start();
        runJob->proc->start(prog,
                            executorArgs(rb.lang, scriptPath,
                                         runJob->binPath));
        if (rb.lang == QLatin1String("sql")) {
            runJob->proc->write(sqliteStdinScript(mdUnescape(rb.code)).toUtf8());
            runJob->proc->closeWriteChannel();
        }
        statusBar()->showMessage(
            QStringLiteral("Ausführung gestartet (%1) …")
                .arg(runLanguageLabel(rb.lang)),
            4000);
        renderMd();
    }


    // -------------------------------------------------------- PDF & printing

    void exportPdf()
    {
        Editor *ed = currentEditor();
        if (!ed)
            return;
        const QString base = ed->filepath.isEmpty()
                                 ? QStringLiteral("Ausgabe")
                                 : QFileInfo(ed->filepath).completeBaseName();
        const QString start = settings.value(
                                  QStringLiteral("state/lastdir"),
                                  QDir::homePath()).toString()
                              + QLatin1Char('/') + base
                              + QStringLiteral(".pdf");
        const QString out = QFileDialog::getSaveFileName(
            this, QStringLiteral("Als PDF exportieren"), start,
            QStringLiteral("PDF (*.pdf)"));
        if (out.isEmpty())
            return;
        exportEditorToPdf(ed, out);
    }

    void exportEditorToPdf(Editor *ed, const QString &outPath)
    {
        const bool isMd = ed->lang == QLatin1String("Markdown");
        QString html;
        QString css;
        if (isMd) {
            html = markdownToHtml(ed->text());
            css = mdCss(false);
        } else {
            html = plainTextHtml(ed->text());
            css = QStringLiteral("pre { font-family: monospace; "
                                  "font-size: 10pt; }");
        }
        QString err;
        if (writePdfFromHtml(html, css, outPath, &err))
            statusBar()->showMessage(
                QStringLiteral("PDF exportiert: %1").arg(outPath), 5000);
        else
            QMessageBox::warning(
                this, QStringLiteral("PDF-Export"),
                QStringLiteral("Export fehlgeschlagen: %1").arg(err));
    }

    void printDocument()
    {
        Editor *ed = currentEditor();
        if (!ed)
            return;
        QPrinter printer(QPrinter::HighResolution);
        QPrintDialog dialog(&printer, this);
        if (dialog.exec() != QDialog::Accepted)
            return;
        const bool isMd = ed->lang == QLatin1String("Markdown");
        QTextDocument doc;
        doc.setDefaultStyleSheet(isMd ? mdCss(false)
            : QStringLiteral("pre { font-family: monospace; "
                             "font-size: 8pt; }"));
        doc.setHtml(isMd ? markdownToHtml(ed->text())
                         : plainTextHtml(ed->text()));
        doc.setPageSize(printer.pageLayout()
                            .paintRectPixels(printer.resolution()).size());
        doc.print(&printer);
    }

    // ------------------------------------------ Find all & find in files

    QRegularExpression buildSearchPattern(const QString &text,
                                          bool caseSensitive)
    {
        return QRegularExpression(QRegularExpression::escape(text),
            caseSensitive ? QRegularExpression::NoPatternOption
                          : QRegularExpression::CaseInsensitiveOption);
    }

    void findAllInDocument()
    {
        Editor *ed = currentEditor();
        if (!ed)
            return;
        if (!findDialog)
            showFindDialog();
        const QString needle = findDialog ? findDialog->editFind->text()
                                           : QString();
        if (needle.isEmpty()) {
            statusBar()->showMessage(
                QStringLiteral("Kein Suchbegriff eingegeben."), 4000);
            return;
        }

        resultsTree->clear();
        auto *rootItem = new QTreeWidgetItem(resultsTree);
        rootItem->setText(0, QStringLiteral("Dokument"));
        rootItem->setToolTip(0, ed->filepath.isEmpty()
                                    ? QStringLiteral("Unbenannt")
                                    : ed->filepath);
        rootItem->setData(0, Qt::UserRole, QString());
        rootItem->setData(0, Qt::UserRole + 1, -1);

        const QRegularExpression re = buildSearchPattern(
            needle, findDialog->checkCase->isChecked());
        int matches = 0;
        for (int line = 0; line < ed->lines() && matches < 1000; ++line) {
            const QString text = ed->text(line);
            QRegularExpressionMatchIterator it = re.globalMatch(text);
            while (it.hasNext() && matches < 1000) {
                const QRegularExpressionMatch m = it.next();
                auto *item = new QTreeWidgetItem(rootItem);
                item->setText(0, QStringLiteral("Zeile %1, Spalte %2")
                                  .arg(line + 1).arg(m.capturedStart() + 1));
                item->setText(1, text.trimmed());
                item->setData(0, Qt::UserRole, QString());
                item->setData(0, Qt::UserRole + 1, line);
                ++matches;
            }
        }
        rootItem->setExpanded(true);
        if (dockResults->isHidden())
            dockResults->show();
        resultsHeader->setText(
            QStringLiteral("Alle suchen: %1 Treffer im Dokument").arg(matches));
        statusBar()->showMessage(
            QStringLiteral("%1 Treffer im Dokument.").arg(matches), 5000);
    }

    void findInFiles()
    {
        QDialog dialog(this);
        dialog.setWindowTitle(QStringLiteral("In Dateien suchen"));
        dialog.setModal(true);
        dialog.setMinimumWidth(540);
        auto *grid = new QGridLayout(&dialog);

        auto *editPattern = new QLineEdit(&dialog);
        editPattern->setPlaceholderText(QStringLiteral("Suchen nach"));
        if (findDialog && !findDialog->editFind->text().isEmpty())
            editPattern->setText(findDialog->editFind->text());
        else if (Editor *ed = currentEditor(); ed && ed->hasSelectedText())
            editPattern->setText(ed->selectedText());

        auto *editDir = new QLineEdit(&dialog);
        editDir->setText(settings.value(QStringLiteral("state/lastdir"),
                                        QDir::homePath()).toString());
        auto *btnBrowse = new QPushButton(QStringLiteral("Durchsuchen ..."),
                                           &dialog);
        connect(btnBrowse, &QPushButton::clicked, this, [&dialog, editDir]{
            const QString dir = QFileDialog::getExistingDirectory(
                &dialog, QStringLiteral("Ordner waehlen"), editDir->text());
            if (!dir.isEmpty())
                editDir->setText(dir);
        });

        auto *editFilter = new QLineEdit(&dialog);
        editFilter->setText(QStringLiteral("*"));
        editFilter->setPlaceholderText(QStringLiteral("z. B. *.md, *.txt"));

        auto *checkCase = new QCheckBox(
            QStringLiteral("Gross-/Kleinschreibung beachten"), &dialog);
        auto *checkSub = new QCheckBox(
            QStringLiteral("Unterverzeichnisse durchsuchen"), &dialog);
        checkSub->setChecked(true);

        auto *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        grid->addWidget(new QLabel(QStringLiteral("Suchen nach:"), &dialog), 0, 0);
        grid->addWidget(editPattern, 0, 1, 1, 2);
        grid->addWidget(new QLabel(QStringLiteral("Ordner:"), &dialog), 1, 0);
        grid->addWidget(editDir, 1, 1);
        grid->addWidget(btnBrowse, 1, 2);
        grid->addWidget(new QLabel(QStringLiteral("Dateifilter:"), &dialog), 2, 0);
        grid->addWidget(editFilter, 2, 1, 1, 2);
        grid->addWidget(checkCase, 3, 0, 1, 3);
        grid->addWidget(checkSub, 4, 0, 1, 3);
        grid->addWidget(buttons, 5, 0, 1, 3);

        if (dialog.exec() != QDialog::Accepted)
            return;
        runFindInFiles(editPattern->text(), editDir->text(),
                       editFilter->text(), checkCase->isChecked(),
                       checkSub->isChecked());
    }

    void runFindInFiles(const QString &needle, const QString &folder,
                        const QString &filter, bool caseSensitive,
                        bool recursive)
    {
        if (needle.isEmpty() || folder.isEmpty() || !QFile::exists(folder)) {
            statusBar()->showMessage(
                QStringLiteral("In Dateien suchen: ungueltige Eingabe."), 5000);
            return;
        }
        settings.setValue(QStringLiteral("state/lastdir"), folder);

        const QRegularExpression re = buildSearchPattern(needle, caseSensitive);
        QStringList nameFilters;
        for (const QString &f
             : filter.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
            const QString t = f.trimmed();
            if (!t.isEmpty())
                nameFilters.append(t);
        }
        if (nameFilters.isEmpty())
            nameFilters.append(QStringLiteral("*"));

        resultsTree->clear();
        int matchCount = 0;
        int fileCount = 0;

        QDirIterator it(folder, nameFilters, QDir::Files,
                       recursive ? QDirIterator::Subdirectories
                                 : QDirIterator::NoIteratorFlags);
        while (it.hasNext() && matchCount < 500) {
            const QString path = it.next();
            const QFileInfo info(path);
            if (info.size() > 8LL * 1024 * 1024)
                continue; // skip large files
            const QString content = readTextFileBest(path);
            if (content.contains(QLatin1Char('\0')))
                continue; // skip binary files

            QTreeWidgetItem *itemForFile = nullptr;
            const QStringList fileLines = content.split(QLatin1Char('\n'));
            for (int line = 0;
                 line < fileLines.size() && matchCount < 500; ++line) {
                auto mt = re.globalMatch(fileLines.at(line));
                while (mt.hasNext() && matchCount < 500) {
                    mt.next();
                    if (!itemForFile) {
                        itemForFile = new QTreeWidgetItem(resultsTree);
                        itemForFile->setText(0, info.fileName());
                        itemForFile->setToolTip(0, path);
                        itemForFile->setText(1, info.dir().path());
                        itemForFile->setData(0, Qt::UserRole, path);
                        itemForFile->setData(0, Qt::UserRole + 1, -1);
                        ++fileCount;
                    }
                    auto *hit = new QTreeWidgetItem(itemForFile);
                    hit->setText(0, QStringLiteral("Zeile %1").arg(line + 1));
                    hit->setText(1, fileLines.at(line).trimmed());
                    hit->setData(0, Qt::UserRole, path);
                    hit->setData(0, Qt::UserRole + 1, line);
                    ++matchCount;
                }
            }
        }

        if (dockResults->isHidden())
            dockResults->show();
        resultsHeader->setText(
            QStringLiteral("In Dateien suchen: %1 Treffer in %2 Dateien")
                .arg(matchCount).arg(fileCount));
        statusBar()->showMessage(
            QStringLiteral("%1 Treffer in %2 Dateien.")
                .arg(matchCount).arg(fileCount), 5000);
    }

    // ---------------------------------------------------- Line operations

    void selectWholeLines(Editor *ed)
    {
        if (!ed || !ed->hasSelectedText())
            return;
        int lineFrom = 0, indexFrom = 0, lineTo = 0, indexTo = 0;
        ed->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
        QString last = ed->text(lineTo);
        while (!last.isEmpty() && (last.endsWith(QLatin1Char('\n'))
                                   || last.endsWith(QLatin1Char('\r'))))
            last.chop(1);
        ed->setSelection(lineFrom, 0, lineTo, last.length());
    }

    void duplicateLine()
    {
        Editor *ed = currentEditor();
        if (!ed)
            return;
        int lineFrom = 0, indexFrom = 0, lineTo = 0, indexTo = 0;
        bool block = false;
        if (ed->hasSelectedText()) {
            ed->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
            block = (lineTo > lineFrom);
        }
        if (block) {
            const QString eol = eolString(ed);
            QStringList blockLines;
            for (int l = lineFrom; l <= lineTo; ++l) {
                QString t = ed->text(l);
                while (!t.isEmpty() && (t.endsWith(QLatin1Char('\n'))
                                       || t.endsWith(QLatin1Char('\r'))))
                    t.chop(1);
                blockLines.append(t);
            }
            ed->beginUndoAction();
            ed->insertAt(blockLines.join(eol) + eol, lineTo + 1, 0);
            ed->endUndoAction();
            return;
        }
        ed->SendScintilla(QsciScintillaBase::SCI_LINEDUPLICATE);
    }

    void deleteLine()
    {
        Editor *ed = currentEditor();
        if (!ed)
            return;
        int lineFrom = 0, indexFrom = 0, lineTo = 0, indexTo = 0;
        if (ed->hasSelectedText()) {
            ed->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
            if (lineTo > lineFrom) {
                QString lastRaw = ed->text(lineTo);
                while (!lastRaw.isEmpty() && (lastRaw.endsWith(QLatin1Char('\n'))
                                              || lastRaw.endsWith(QLatin1Char('\r'))))
                    lastRaw.chop(1);
                const int lastLineIndex = ed->lines() - 1;
                ed->beginUndoAction();
                if (lineTo < lastLineIndex) {
                    // including the line ending of the last block line
                    ed->setSelection(lineFrom, 0, lineTo + 1, 0);
                } else if (lineFrom > 0) {
                    // block ends at end of file: remove the preceding EOL
                    QString prev = ed->text(lineFrom - 1);
                    while (!prev.isEmpty() && (prev.endsWith(QLatin1Char('\n'))
                                               || prev.endsWith(QLatin1Char('\r'))))
                        prev.chop(1);
                    ed->setSelection(lineFrom - 1, prev.length(),
                                     lineTo, lastRaw.length());
                } else {
                    // entire document selected
                    ed->setSelection(lineFrom, 0, lineTo, lastRaw.length());
                }
                ed->replaceSelectedText(QString());
                ed->endUndoAction();
                return;
            }
        }
        ed->SendScintilla(QsciScintillaBase::SCI_LINEDELETE);
    }

    void moveLineUp()
    {
        if (Editor *ed = currentEditor())
            ed->SendScintilla(QsciScintillaBase::SCI_MOVESELECTEDLINESUP);
    }

    void moveLineDown()
    {
        if (Editor *ed = currentEditor())
            ed->SendScintilla(QsciScintillaBase::SCI_MOVESELECTEDLINESDOWN);
    }

    QString eolString(Editor *ed) const
    {
        switch (ed->eolMode()) {
        case QsciScintilla::EolWindows: return QStringLiteral("\r\n");
        case QsciScintilla::EolMac:    return QStringLiteral("\r");
        default:                        return QStringLiteral("\n");
        }
    }

    void transformSelectedLines(
        const std::function<QStringList(const QStringList &)> &transform)
    {
        Editor *ed = currentEditor();
        if (!ed)
            return;
        int lineFrom = 0, indexFrom = 0, lineTo = 0, indexTo = 0;
        if (ed->hasSelectedText()) {
            ed->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
        } else {
            lineTo = ed->lines() - 1;
        }
        if (lineTo < lineFrom)
            return;

        QStringList linesList;
        auto stripEol = [](QString t) {
            while (!t.isEmpty() && (t.endsWith(QLatin1Char('\n'))
                                   || t.endsWith(QLatin1Char('\r'))))
                t.chop(1);
            return t;
        };
        for (int l = lineFrom; l <= lineTo; ++l)
            linesList.append(stripEol(ed->text(l)));
        linesList = transform(linesList);
        if (linesList.isEmpty())
            linesList.append(QString());

        ed->beginUndoAction();
        ed->setSelection(lineFrom, 0, lineTo, stripEol(ed->text(lineTo)).length());
        ed->replaceSelectedText(linesList.join(eolString(ed)));
        ed->setSelection(lineFrom, 0, lineTo, linesList.last().length());
        ed->endUndoAction();
    }

    void sortLinesAsc()
    {
        transformSelectedLines([](QStringList ls) {
            std::sort(ls.begin(), ls.end(),
                      [](const QString &a, const QString &b) {
                          return a.compare(b, Qt::CaseInsensitive) < 0;
                      });
            return ls;
        });
    }

    void sortLinesDesc()
    {
        transformSelectedLines([](QStringList ls) {
            std::sort(ls.begin(), ls.end(),
                      [](const QString &a, const QString &b) {
                          return a.compare(b, Qt::CaseInsensitive) > 0;
                      });
            return ls;
        });
    }

    void uniqueLines()
    {
        transformSelectedLines([](QStringList ls) {
            std::sort(ls.begin(), ls.end());
            ls.erase(std::unique(ls.begin(), ls.end()), ls.end());
            return ls;
        });
    }

    void trimTrailing()
    {
        transformSelectedLines([](QStringList ls) {
            for (QString &ln : ls) {
                while (!ln.isEmpty()
                       && (ln.endsWith(QLatin1Char(' '))
                           || ln.endsWith(QLatin1Char('\t')))) {
                    ln.chop(1);
                }
            }
            return ls;
        });
    }

    void trimBoth()
    {
        transformSelectedLines([](QStringList ls) {
            for (QString &ln : ls)
                ln = ln.trimmed();
            return ls;
        });
    }

    // --------------------------------------------- Upper/lower case

    static QString titleCaseText(const QString &s)
    {
        QString r(s);
        bool atWordStart = true;
        for (QChar &ch : r) {
            if (ch.isLetter()) {
                ch = atWordStart ? ch.toUpper() : ch.toLower();
                atWordStart = ch.isSpace();
            } else if (ch.isSpace()) {
                atWordStart = true;
            }
        }
        return r;
    }

    void convertCase(int mode) // 0 = UPPER, 1 = lower, 2 = Title Case
    {
        Editor *ed = currentEditor();
        if (!ed)
            return;
        if (ed->hasSelectedText()) {
            const QString sel = ed->selectedText();
            ed->beginUndoAction();
            ed->replaceSelectedText(mode == 0 ? sel.toUpper()
                                : mode == 1 ? sel.toLower()
                                            : titleCaseText(sel));
            ed->endUndoAction();
            return;
        }
        int line = 0, index = 0;
        ed->getCursorPosition(&line, &index);
        const QString lineText = ed->text(line);
        int from = index, to = index;
        while (from > 0 && lineText.at(from - 1).isLetterOrNumber())
            --from;
        while (to < lineText.size() && lineText.at(to).isLetterOrNumber())
            ++to;
        if (from == to)
            return;
        const QString word = lineText.mid(from, to - from);
        const QString conv = mode == 0 ? word.toUpper()
                              : mode == 1 ? word.toLower()
                                          : titleCaseText(word);
        ed->beginUndoAction();
        ed->setSelection(line, from, line, to);
        ed->replaceSelectedText(conv);
        ed->endUndoAction();
    }

    void upperCase() { convertCase(0); }
    void lowerCase() { convertCase(1); }
    void titleCase() { convertCase(2); }

    // ---------------------------------------------------- Line comment

    static QString commentTokenFor(const QString &lang)
    {
        if (lang == QLatin1String("Python") || lang == QLatin1String("Bash")
            || lang == QLatin1String("Ruby") || lang == QLatin1String("YAML")
            || lang == QLatin1String("INI")
            || lang == QLatin1String("Makefile")
            || lang == QLatin1String("CMake"))
            return QStringLiteral("#");
        if (lang == QLatin1String("C++") || lang == QLatin1String("Java")
            || lang == QLatin1String("JavaScript")
            || lang == QLatin1String("C#") || lang == QLatin1String("CSS")
            || lang == QLatin1String("JSON"))
            return QStringLiteral("//");
        if (lang == QLatin1String("SQL") || lang == QLatin1String("Lua"))
            return QStringLiteral("--");
        return QString();
    }

    void toggleLineComment()
    {
        Editor *ed = currentEditor();
        if (!ed)
            return;
        const QString token = commentTokenFor(ed->lang);
        if (token.isEmpty()) {
            statusBar()->showMessage(
                QStringLiteral("Fuer diese Sprache ist kein "
                               "Zeilenkommentar definiert."), 4000);
            return;
        }

        int lineFrom = 0, indexFrom = 0, lineTo = 0, indexTo = 0;
        if (ed->hasSelectedText()) {
            ed->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
        } else {
            ed->getCursorPosition(&lineFrom, &indexFrom);
            lineTo = lineFrom;
        }

        bool allCommented = true;
        for (int l = lineFrom; l <= lineTo; ++l) {
            QString lnR = ed->text(l);
            while (!lnR.isEmpty() && (lnR.endsWith(QLatin1Char('\n'))
                                      || lnR.endsWith(QLatin1Char('\r'))))
                lnR.chop(1);
            const QString ln = lnR.trimmed();
            if (!ln.isEmpty() && !ln.startsWith(token)) {
                allCommented = false;
                break;
            }
        }

        QStringList result;
        for (int l = lineFrom; l <= lineTo; ++l) {
            QString ln = ed->text(l);
            while (!ln.isEmpty() && (ln.endsWith(QLatin1Char('\n'))
                                    || ln.endsWith(QLatin1Char('\r'))))
                ln.chop(1);
            if (allCommented) {
                const int idx = ln.indexOf(token);
                if (idx >= 0) {
                    ln.remove(idx, token.size());
                    if (ln.size() > idx && ln.at(idx) == QLatin1Char(' '))
                        ln.remove(idx, 1);
                }
            } else {
                int lead = 0;
                while (lead < ln.size() && ln.at(lead).isSpace())
                    ++lead;
                if (lead == ln.size())
                    { result.append(ln); continue; } // empty line untouched
                ln.insert(lead, token + QLatin1Char(' '));
            }
            result.append(ln);
        }

        QString lastLine = ed->text(lineTo);
        while (!lastLine.isEmpty() && (lastLine.endsWith(QLatin1Char('\n'))
                                      || lastLine.endsWith(QLatin1Char('\r'))))
            lastLine.chop(1);
        ed->beginUndoAction();
        ed->setSelection(lineFrom, 0, lineTo, lastLine.length());
        ed->replaceSelectedText(result.join(eolString(ed)));
        ed->setSelection(lineFrom, 0, lineTo, result.last().length());
        ed->endUndoAction();
    }

    // -------------------------------------------------------------- Bookmarks

    void toggleBookmarkAt(Editor *ed, int line)
    {
        if (!ed || ed->bmMarker < 0)
            return;
        if (ed->markersAtLine(line) & (1u << ed->bmMarker))
            ed->markerDelete(line, ed->bmMarker);
        else
            ed->markerAdd(line, ed->bmMarker);
    }

    void toggleBookmarkCurrent()
    {
        Editor *ed = currentEditor();
        if (!ed)
            return;
        int line = 0, index = 0;
        ed->getCursorPosition(&line, &index);
        toggleBookmarkAt(ed, line);
    }

    void gotoBookmark(bool forward)
    {
        Editor *ed = currentEditor();
        if (!ed || ed->bmMarker < 0)
            return;
        int line = 0, index = 0;
        ed->getCursorPosition(&line, &index);
        const int mask = 1 << ed->bmMarker;
        int target = -1;
        if (forward) {
            target = ed->markerFindNext(line + 1, mask);
            if (target < 0)
                target = ed->markerFindNext(0, mask);
        } else {
            target = ed->markerFindPrevious(line - 1, mask);
            if (target < 0)
                target = ed->markerFindPrevious(ed->lines() - 1, mask);
        }
        if (target < 0)
            return;
        ed->setCursorPosition(target, 0);
        ed->ensureLineVisible(target);
    }

    void clearBookmarks()
    {
        Editor *ed = currentEditor();
        if (ed && ed->bmMarker >= 0)
            ed->markerDeleteAll(ed->bmMarker);
    }

    // ------------------------------------------------- Document statistics

    void documentStats()
    {
        Editor *ed = currentEditor();
        if (!ed)
            return;
        const QString text = ed->text();
        static const QRegularExpression wordRe(QStringLiteral("\\S+"));
        int words = 0;
        auto wt = wordRe.globalMatch(text);
        while (wt.hasNext()) { wt.next(); ++words; }

        QString selInfo;
        if (ed->hasSelectedText()) {
            const QString sel = ed->selectedText();
            int selWords = 0;
            auto st = wordRe.globalMatch(sel);
            while (st.hasNext()) { st.next(); ++selWords; }
            selInfo = QStringLiteral("<p>Auswahl: %1 Zeichen, %2 Woerter</p>")
                          .arg(sel.size()).arg(selWords);
        }

        QMessageBox::information(
            this, QStringLiteral("Dokumentstatistik"),
            QStringLiteral("<p>Zeilen: %1</p><p>Woerter: %2</p>"
                           "<p>Zeichen: %3</p>%4")
                .arg(ed->lines()).arg(words).arg(text.size()).arg(selInfo));
    }

private:
    QAction *mkAction(const QString &text,
                      const QKeySequence &shortcut = QKeySequence())
    {
        auto *act = new QAction(text, this);
        act->setShortcut(shortcut);
        return act;
    }

    void buildMenus()
    {
        QMenuBar *bar = menuBar();

        // -- File ---------------------------------------------------------------
        QMenu *menuFile = bar->addMenu(QStringLiteral("Datei"));

        QAction *actNew = mkAction(QStringLiteral("Neu"), QKeySequence::New);
        connect(actNew, &QAction::triggered, this, [this]{ newDocument(); });
        actNew->setIcon(style()->standardIcon(QStyle::SP_FileIcon));

        QAction *actOpen = mkAction(QStringLiteral("Öffnen ..."), QKeySequence::Open);
        connect(actOpen, &QAction::triggered, this, [this]{ openFilesDialog(); });
        actOpen->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));

        recentMenu = menuFile->addMenu(QStringLiteral("Zuletzt geöffnet"));

        QAction *actSave = mkAction(QStringLiteral("Speichern"), QKeySequence::Save);
        connect(actSave, &QAction::triggered, this, [this]{ saveCurrent(); });
        actSave->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));

        QAction *actSaveAs = mkAction(QStringLiteral("Speichern unter ..."),
                                      QKeySequence(QStringLiteral("Ctrl+Shift+S")));
        connect(actSaveAs, &QAction::triggered, this, [this]{ saveCurrentAs(); });

        QAction *actSaveAll = mkAction(QStringLiteral("Alle speichern"),
                                       QKeySequence(QStringLiteral("Ctrl+Alt+S")));
        connect(actSaveAll, &QAction::triggered, this, [this]{ saveAll(); });

        QAction *actExportPdf =
            mkAction(QStringLiteral("Als PDF exportieren ..."));
        connect(actExportPdf, &QAction::triggered, this, [this]{ exportPdf(); });
        actExportPdf->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));

        QAction *actPrint = mkAction(QStringLiteral("Drucken ..."),
                                     QKeySequence::Print);
        connect(actPrint, &QAction::triggered, this, [this]{ printDocument(); });

        QAction *actClose = mkAction(QStringLiteral("Schließen"),
                                      QKeySequence(QStringLiteral("Ctrl+W")));
        connect(actClose, &QAction::triggered, this,
                [this]{ closeTab(tabs->currentIndex()); });

        QAction *actCloseAll = mkAction(QStringLiteral("Alle schließen"),
                                        QKeySequence(QStringLiteral("Ctrl+Shift+W")));
        connect(actCloseAll, &QAction::triggered, this, [this]{ closeAll(); });

        QAction *actQuit = mkAction(QStringLiteral("Beenden"),
                                     QKeySequence(QStringLiteral("Ctrl+Shift+Q")));
        connect(actQuit, &QAction::triggered, this, [this]{ close(); });

        menuFile->addAction(actNew);
        menuFile->addAction(actOpen);
        menuFile->addAction(recentMenu->menuAction());
        menuFile->addSeparator();
        menuFile->addAction(actSave);
        menuFile->addAction(actSaveAs);
        menuFile->addAction(actSaveAll);
        menuFile->addAction(actExportPdf);
        menuFile->addAction(actPrint);
        menuFile->addSeparator();
        menuFile->addAction(actClose);
        menuFile->addAction(actCloseAll);
        menuFile->addSeparator();
        menuFile->addAction(actQuit);

        // -- Edit ------------------------------------------------------------------
        QMenu *menuEdit = bar->addMenu(QStringLiteral("Bearbeiten"));
        struct EditOp {
            QString label;
            void (QsciScintilla::*member)();
            QKeySequence shortcut;
        };
        const QVector<EditOp> editOps = {
            {QStringLiteral("Rückgängig"),  &QsciScintilla::undo, QKeySequence::Undo},
            {QStringLiteral("Wiederholen"), &QsciScintilla::redo, QKeySequence::Redo},
            {QStringLiteral("Ausschneiden"), &QsciScintilla::cut,  QKeySequence::Cut},
            {QStringLiteral("Kopieren"),    &QsciScintilla::copy, QKeySequence::Copy},
            {QStringLiteral("Einfügen"),    &QsciScintilla::paste, QKeySequence::Paste},
        };
        for (const EditOp &op : editOps) {
            QAction *act = menuEdit->addAction(op.label);
            act->setShortcut(op.shortcut);
            connect(act, &QAction::triggered, this, [this, member = op.member]{
                if (Editor *ed = currentEditor())
                    (ed->*member)();
            });
        }
        menuEdit->addSeparator();
        QAction *actAll = menuEdit->addAction(QStringLiteral("Alles auswählen"));
        actAll->setShortcut(QKeySequence(QStringLiteral("Ctrl+A")));
        connect(actAll, &QAction::triggered, this, [this]{
            if (Editor *ed = currentEditor())
                ed->selectAll(true);
        });

        // Line operations
        auto addSimple = [this](QMenu *menu, const QString &label,
                                const QString &shortcut,
                                void (MainWindow::*fn)()) {
            QAction *act = menu->addAction(label);
            if (!shortcut.isEmpty())
                act->setShortcut(QKeySequence(shortcut));
            connect(act, &QAction::triggered, this, [this, fn]{
                (this->*fn)();
            });
            return act;
        };
        QMenu *menuLines =
            menuEdit->addMenu(QStringLiteral("Zeilenoperationen"));
        addSimple(menuLines, QStringLiteral("Zeile duplizieren"),
                  QStringLiteral("Ctrl+D"), &MainWindow::duplicateLine);
        addSimple(menuLines, QStringLiteral("Zeile loeschen"),
                  QStringLiteral("Ctrl+L"), &MainWindow::deleteLine);
        addSimple(menuLines, QStringLiteral("Zeile nach oben verschieben"),
                  QStringLiteral("Ctrl+Shift+Up"), &MainWindow::moveLineUp);
        addSimple(menuLines, QStringLiteral("Zeile nach unten verschieben"),
                  QStringLiteral("Ctrl+Shift+Down"), &MainWindow::moveLineDown);
        menuLines->addSeparator();
        addSimple(menuLines, QStringLiteral("Zeilen aufsteigend sortieren"),
                  QString(), &MainWindow::sortLinesAsc);
        addSimple(menuLines, QStringLiteral("Zeilen absteigend sortieren"),
                  QString(), &MainWindow::sortLinesDesc);
        addSimple(menuLines, QStringLiteral("Doppelte Zeilen entfernen"),
                  QString(), &MainWindow::uniqueLines);
        menuLines->addSeparator();
        addSimple(menuLines,
                  QStringLiteral("Nachfolgende Leerzeichen entfernen"),
                  QString(), &MainWindow::trimTrailing);
        addSimple(menuLines,
                  QStringLiteral("Leerzeichen an beiden Enden entfernen"),
                  QString(), &MainWindow::trimBoth);

        QMenu *menuCase =
            menuEdit->addMenu(QStringLiteral("Gross-/Kleinschreibung"));
        addSimple(menuCase, QStringLiteral("GROSSBUCHSTABEN"),
                  QStringLiteral("Ctrl+Shift+U"), &MainWindow::upperCase);
        addSimple(menuCase, QStringLiteral("kleinbuchstaben"),
                  QStringLiteral("Ctrl+U"), &MainWindow::lowerCase);
        addSimple(menuCase, QStringLiteral("Titelschreibweise"),
                  QString(), &MainWindow::titleCase);

        menuEdit->addSeparator();
        QAction *actComment = mkAction(
            QStringLiteral("Zeilenkommentar umschalten (Ctrl+Q)"),
            QKeySequence(QStringLiteral("Ctrl+Q")));
        connect(actComment, &QAction::triggered, this,
                [this]{ toggleLineComment(); });
        menuEdit->addAction(actComment);

        // -- Search -----------------------------------------------------------------
        QMenu *menuSearch = bar->addMenu(QStringLiteral("Suche"));
        QAction *actFind = menuSearch->addAction(QStringLiteral("Suchen ..."));
        actFind->setShortcut(QKeySequence(QStringLiteral("Ctrl+F")));
        connect(actFind, &QAction::triggered, this, [this]{ showFindDialog(); });

        QAction *actFindNext = menuSearch->addAction(QStringLiteral("Weitersuchen"));
        actFindNext->setShortcut(QKeySequence(QStringLiteral("F3")));
        connect(actFindNext, &QAction::triggered, this, [this]{ findStep(false); });

        QAction *actFindPrev = menuSearch->addAction(QStringLiteral("Rückwärts suchen"));
        actFindPrev->setShortcut(QKeySequence(QStringLiteral("Shift+F3")));
        connect(actFindPrev, &QAction::triggered, this, [this]{ findStep(true); });

        QAction *actReplace = menuSearch->addAction(QStringLiteral("Ersetzen ..."));
        actReplace->setShortcut(QKeySequence(QStringLiteral("Ctrl+H")));
        connect(actReplace, &QAction::triggered, this, [this]{ showFindDialog(); });

        menuSearch->addSeparator();
        QAction *actGoto = menuSearch->addAction(QStringLiteral("Gehe zu Zeile ..."));
        actGoto->setShortcut(QKeySequence(QStringLiteral("Ctrl+G")));
        connect(actGoto, &QAction::triggered, this, [this]{ gotoLine(); });

        menuSearch->addSeparator();
        QAction *actFindAll =
            menuSearch->addAction(QStringLiteral("Alle suchen ..."));
        connect(actFindAll, &QAction::triggered, this,
                [this]{ findAllInDocument(); });

        QAction *actInFiles =
            menuSearch->addAction(QStringLiteral("In Dateien suchen ..."));
        actInFiles->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+F")));
        connect(actInFiles, &QAction::triggered, this,
                [this]{ findInFiles(); });

        menuSearch->addSeparator();
        QMenu *menuBookmarks =
            menuSearch->addMenu(QStringLiteral("Lesezeichen"));
        QAction *actBmToggle = menuBookmarks->addAction(
            QStringLiteral("Lesezeichen umschalten"));
        actBmToggle->setShortcut(QKeySequence(QStringLiteral("Ctrl+F2")));
        connect(actBmToggle, &QAction::triggered, this,
                [this]{ toggleBookmarkCurrent(); });
        QAction *actBmNext = menuBookmarks->addAction(
            QStringLiteral("Naechstes Lesezeichen"));
        actBmNext->setShortcut(QKeySequence(QStringLiteral("F2")));
        connect(actBmNext, &QAction::triggered, this,
                [this]{ gotoBookmark(true); });
        QAction *actBmPrev = menuBookmarks->addAction(
            QStringLiteral("Vorheriges Lesezeichen"));
        actBmPrev->setShortcut(QKeySequence(QStringLiteral("Shift+F2")));
        connect(actBmPrev, &QAction::triggered, this,
                [this]{ gotoBookmark(false); });
        QAction *actBmClear = menuBookmarks->addAction(
            QStringLiteral("Alle Lesezeichen loeschen"));
        connect(actBmClear, &QAction::triggered, this,
                [this]{ clearBookmarks(); });

        menuSearch->addSeparator();
        QAction *actStats =
            menuSearch->addAction(QStringLiteral("Dokumentstatistik ..."));
        connect(actStats, &QAction::triggered, this,
                [this]{ documentStats(); });

        // -- View --------------------------------------------------------------------
        QMenu *menuView = bar->addMenu(QStringLiteral("Ansicht"));

        QAction *actZoomIn = menuView->addAction(QStringLiteral("Vergrößern"));
        actZoomIn->setShortcut(QKeySequence(QStringLiteral("Ctrl++")));
        connect(actZoomIn, &QAction::triggered, this, [this]{ zoom(1); });

        QAction *actZoomOut = menuView->addAction(QStringLiteral("Verkleinern"));
        actZoomOut->setShortcut(QKeySequence(QStringLiteral("Ctrl+-")));
        connect(actZoomOut, &QAction::triggered, this, [this]{ zoom(-1); });

        QAction *actZoomReset = menuView->addAction(QStringLiteral("Zoom zurücksetzen"));
        actZoomReset->setShortcut(QKeySequence(QStringLiteral("Ctrl+0")));
        connect(actZoomReset, &QAction::triggered, this, [this]{ zoom(0, true); });

        menuView->addSeparator();

        QAction *actMdDockAction =
            menuView->addAction(QStringLiteral("Markdown-Ansicht"));
        actMdDockAction->setCheckable(true);
        actMdDockAction->setChecked(false);
        actMdDockAction->setShortcut(QKeySequence(QStringLiteral("F9")));
        actMdDock = actMdDockAction;
        connect(actMdDockAction, &QAction::toggled, this, [this](bool on){
            if (dockMd->isVisible() != on) {
                const QSignalBlocker guard(dockMd);
                dockMd->setVisible(on);
            }
            if (on)
                renderMd();
        });

        QAction *actFullScreenFull =
            menuView->addAction(QStringLiteral("Vollbild"));
        actFullScreenFull->setCheckable(true);
        actFullScreenFull->setChecked(isFullScreen());
        actFullScreenFull->setShortcut(QKeySequence(QStringLiteral("F11")));
        connect(actFullScreenFull, &QAction::toggled, this, [this](bool on){
            if (on)
                showFullScreen();
            else
                showNormal();
        });

        QAction *actOnTop =
            menuView->addAction(QStringLiteral("Immer im Vordergrund"));
        actOnTop->setCheckable(true);
        actOnTop->setChecked(windowFlags().testFlag(Qt::WindowStaysOnTopHint));
        actOnTop->setShortcut(QKeySequence(QStringLiteral("F12")));
        connect(actOnTop, &QAction::toggled, this, [this](bool on){
            if (windowFlags().testFlag(Qt::WindowStaysOnTopHint) == on)
                return;
            const bool shown = isVisible();
            setWindowFlag(Qt::WindowStaysOnTopHint, on);
            setVisible(shown);
        });

        menuView->addSeparator();

        QAction *actLineno = menuView->addAction(QStringLiteral("Zeilennummern"));
        actLineno->setCheckable(true);
        actLineno->setChecked(flagLineno);
        connect(actLineno, &QAction::toggled, this, [this](bool on){
            flagLineno = on;
            for (Editor *ed : editors)
                ed->setMarginLineNumbers(0, on);
        });

        QAction *actWrap = menuView->addAction(QStringLiteral("Zeilenumbruch"));
        actWrap->setCheckable(true);
        actWrap->setChecked(flagWrap);
        connect(actWrap, &QAction::toggled, this, [this](bool on){
            flagWrap = on;
            for (Editor *ed : editors)
                ed->setWrapMode(on ? QsciScintilla::WrapWord
                                   : QsciScintilla::WrapNone);
        });

        QAction *actWs = menuView->addAction(QStringLiteral("Leerzeichen anzeigen"));
        actWs->setCheckable(true);
        actWs->setChecked(flagWs);
        connect(actWs, &QAction::toggled, this, [this](bool on){
            flagWs = on;
            for (Editor *ed : editors)
                ed->SendScintilla(QsciScintillaBase::SCI_SETVIEWWS, on ? 1 : 0);
        });

        QAction *actGuides = menuView->addAction(QStringLiteral("Einrückungshilfen"));
        actGuides->setCheckable(true);
        actGuides->setChecked(flagGuides);
        connect(actGuides, &QAction::toggled, this, [this](bool on){
            flagGuides = on;
            for (Editor *ed : editors)
                ed->SendScintilla(
                    QsciScintillaBase::SCI_SETINDENTATIONGUIDES, on ? 3 : 0);
        });

        menuView->addSeparator();

        QMenu *menuTheme = menuView->addMenu(QStringLiteral("Design"));
        auto *themeGroup = new QActionGroup(this);
        const QVector<QPair<bool, QString>> themeEntries = {
            {false, QStringLiteral("Hell")},
            {true,  QStringLiteral("Dunkel")},
        };
        for (const auto &entry : themeEntries) {
            QAction *act = themeGroup->addAction(entry.second);
            act->setCheckable(true);
            act->setProperty("themeDark", entry.first);
            act->setChecked(dark == entry.first);
            connect(act, &QAction::triggered, this,
                    [this, isDark = entry.first]{ setTheme(isDark); });
            menuTheme->addAction(act);
            themeMenuActs.append(act);
        }

        QMenu *menuLang = menuView->addMenu(QStringLiteral("Sprache"));
        auto *langGroup = new QActionGroup(this);
        QAction *actPlain = langGroup->addAction(QStringLiteral("Normaltext"));
        actPlain->setCheckable(true);
        actPlain->setChecked(true);
        connect(actPlain, &QAction::triggered, this, [this]{
            if (Editor *ed = currentEditor())
                applyLexer(ed, QString());
        });
        menuLang->addAction(actPlain);
        menuLang->addSeparator();
        const QStringList languages = {
            QStringLiteral("Python"), QStringLiteral("C++"),
            QStringLiteral("C#"), QStringLiteral("Java"),
            QStringLiteral("JavaScript"), QStringLiteral("HTML"),
            QStringLiteral("XML"), QStringLiteral("CSS"),
            QStringLiteral("Bash"), QStringLiteral("SQL"),
            QStringLiteral("INI"), QStringLiteral("Makefile"),
            QStringLiteral("Lua"), QStringLiteral("Ruby"),
            QStringLiteral("Pascal"), QStringLiteral("Diff"),
#if NPPLX_HAS_JSON
            QStringLiteral("JSON"),
#endif
#if NPPLX_HAS_MARKDOWN
            QStringLiteral("Markdown"),
#endif
#if NPPLX_HAS_YAML
            QStringLiteral("YAML"),
#endif
#if NPPLX_HAS_CMAKE
            QStringLiteral("CMake"),
#endif
        };
        for (const QString &lang : languages) {
            QAction *act = langGroup->addAction(lang);
            act->setCheckable(true);
            connect(act, &QAction::triggered, this, [this, lang]{
                if (Editor *ed = currentEditor())
                    applyLexer(ed, lang);
            });
            menuLang->addAction(act);
        }

        // -- Encoding & line endings ----------------------------------------------
        QMenu *menuEnc = bar->addMenu(QStringLiteral("Codierung"));
        auto *encGroup = new QActionGroup(this);
        const QVector<QPair<QString, QString>> encEntries = {
            {QStringLiteral("utf-8"), QStringLiteral("UTF-8")},
            {QStringLiteral("ansi"),   QStringLiteral("ANSI (ISO-8859-1)")},
        };
        for (const auto &entry : encEntries) {
            QAction *act = encGroup->addAction(entry.second);
            act->setCheckable(true);
            connect(act, &QAction::triggered, this, [this, key = entry.first]{
                if (Editor *ed = currentEditor())
                    ed->encoding = key;
                updateStatus();
            });
            menuEnc->addAction(act);
        }

        QMenu *menuEol = bar->addMenu(QStringLiteral("Zeilenende"));
        auto *eolGroup = new QActionGroup(this);
        const QVector<QPair<QsciScintilla::EolMode, QString>> eolEntries = {
            {QsciScintilla::EolUnix,    QStringLiteral("Unix (LF)")},
            {QsciScintilla::EolWindows, QStringLiteral("Windows (CRLF)")},
            {QsciScintilla::EolMac,    QStringLiteral("Klassisches Mac (CR)")},
        };
        for (const auto &entry : eolEntries) {
            QAction *act = eolGroup->addAction(entry.second);
            act->setCheckable(true);
            connect(act, &QAction::triggered, this,
                    [this, mode = entry.first]{
                        if (Editor *ed = currentEditor()) {
                            ed->convertEols(mode);
                            ed->setEolMode(mode);
                        }
                        updateStatus();
                    });
            menuEol->addAction(act);
        }

        // -- Help --------------------------------------------------------------------
        QMenu *menuTools = bar->addMenu(QStringLiteral("Extras"));
        QAction *actExec = menuTools->addAction(
            QStringLiteral("Werkzeuge für Code-Ausführung …"));
        connect(actExec, &QAction::triggered, this,
                [this]{ showExecDialog(); });

        QMenu *menuHelp = bar->addMenu(QStringLiteral("Hilfe"));
        QAction *actAbout = menuHelp->addAction(QStringLiteral("Über NppLX"));
        connect(actAbout, &QAction::triggered, this, [this]{
            QMessageBox::about(this, QStringLiteral("Über NppLX"),
                QStringLiteral(
                    "<p><b>NppLX</b> ist ein Tabbed-Texteditor im Stil von "
                    "Notepad++ für Linux.</p>"
                    "<p>Editor-Komponente: Scintilla (QScintilla), "
                    "UI-Framework: Qt.</p>"
                    "<p>Lizenz: GPL-3.0-or-later. Unabhängige "
                    "Neuimplementierung; kein Bestandteil von Notepad++ und "
                    "von dessen Herstellern nicht unterstützt.</p>"));
        });

        // -- Toolbar --------------------------------------------------------------------
        QToolBar *toolbar = addToolBar(QStringLiteral("Haupt-Symbolleiste"));
        toolbar->setMovable(false);
        toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        toolbar->addAction(actNew);
        toolbar->addAction(actOpen);
        toolbar->addAction(actSave);
        toolbar->addAction(actSaveAll);
        toolbar->addSeparator();
        toolbar->addAction(actFind);
        toolbar->addAction(actReplace);
        toolbar->addSeparator();
        actThemeToggle = toolbar->addAction(dark
                                                ? QStringLiteral("Hell")
                                                : QStringLiteral("Dunkel"));
        actThemeToggle->setCheckable(true);
        actThemeToggle->setChecked(dark);
        connect(actThemeToggle, &QAction::toggled, this,
                [this](bool on){ setTheme(on); });
    }
};

// ---------------------------------------------------------------------------
// FindReplaceDialog implementation
// ---------------------------------------------------------------------------

FindReplaceDialog::FindReplaceDialog(MainWindow *main)
    : QDialog(main)
    , main(main)
{
    setWindowTitle(QStringLiteral("Suchen / Ersetzen"));
    setModal(false);
    setMinimumWidth(400);

    auto *layout = new QGridLayout(this);
    editFind = new QLineEdit(this);
    editFind->setPlaceholderText(QStringLiteral("Suchen nach"));
    editReplace = new QLineEdit(this);
    editReplace->setPlaceholderText(QStringLiteral("Ersetzen mit"));
    layout->addWidget(new QLabel(QStringLiteral("Suchen:"), this), 0, 0);
    layout->addWidget(editFind, 0, 1);
    layout->addWidget(new QLabel(QStringLiteral("Ersetzen:"), this), 1, 0);
    layout->addWidget(editReplace, 1, 1);

    checkCase = new QCheckBox(QStringLiteral("Groß-/Kleinschreibung"), this);
    checkWord = new QCheckBox(QStringLiteral("Ganzes Wort"), this);
    checkRegex = new QCheckBox(QStringLiteral("Regulärer Ausdruck"), this);
    auto *options = new QHBoxLayout;
    options->addWidget(checkCase);
    options->addWidget(checkWord);
    options->addWidget(checkRegex);
    layout->addLayout(options, 2, 0, 1, 2);

    auto *btnNext = new QPushButton(QStringLiteral("Weitersuchen"), this);
    auto *btnPrev = new QPushButton(QStringLiteral("Rückwärts"), this);
    auto *btnReplace = new QPushButton(QStringLiteral("Ersetzen"), this);
    auto *btnReplaceAll = new QPushButton(QStringLiteral("Alle ersetzen"), this);
    auto *btnClose = new QPushButton(QStringLiteral("Schließen"), this);

    auto *buttons = new QHBoxLayout;
    buttons->addWidget(btnNext);
    buttons->addWidget(btnPrev);
    buttons->addWidget(btnReplace);
    buttons->addWidget(btnReplaceAll);
    buttons->addWidget(btnClose);
    layout->addLayout(buttons, 3, 0, 1, 2);

    connect(btnNext, &QPushButton::clicked, this,
            [this]{ findFrom(false); });
    connect(btnPrev, &QPushButton::clicked, this,
            [this]{ findFrom(true); });
    connect(editFind, &QLineEdit::returnPressed, this,
            [this]{ findFrom(false); });
    connect(btnReplace, &QPushButton::clicked, this,
            [this]{ replaceOne(); });
    connect(btnReplaceAll, &QPushButton::clicked, this,
            [this]{ replaceAll(); });
    connect(btnClose, &QPushButton::clicked, this, &QWidget::hide);

    auto *escape = new QShortcut(QKeySequence(QStringLiteral("Escape")), this);
    connect(escape, &QShortcut::activated, this, &QWidget::hide);
}

void FindReplaceDialog::findFrom(bool backward)
{
    Editor *ed = main->currentEditor();
    if (!ed)
        return;
    const QString pattern = editFind->text();
    if (pattern.isEmpty())
        return;
    int line = 0, index = 0;
    ed->getCursorPosition(&line, &index);
    if (!ed->findFirst(pattern, checkRegex->isChecked(),
                       checkCase->isChecked(), checkWord->isChecked(),
                       false, !backward, line, index))
        main->statusBar()->showMessage(
            QStringLiteral("Keine weiteren Treffer."), 4000);
}

void FindReplaceDialog::replaceOne()
{
    Editor *ed = main->currentEditor();
    if (!ed)
        return;
    if (!(ed->hasSelectedText() && ed->selectedText() == editFind->text())) {
        int line = 0, index = 0;
        ed->getCursorPosition(&line, &index);
        if (!ed->findFirst(editFind->text(), checkRegex->isChecked(),
                           checkCase->isChecked(), checkWord->isChecked(),
                           false, true, line, index)) {
            main->statusBar()->showMessage(
                QStringLiteral("Keine weiteren Treffer."), 4000);
            return;
        }
    }
    ed->replace(editReplace->text());
}

void FindReplaceDialog::replaceAll()
{
    Editor *ed = main->currentEditor();
    if (!ed)
        return;
    const QString pattern = editFind->text();
    if (pattern.isEmpty())
        return;
    int line = 0, index = 0;
    int count = 0;
    ed->beginUndoAction();
    while (count < 100000) {
        if (!ed->findFirst(pattern, checkRegex->isChecked(),
                           checkCase->isChecked(), checkWord->isChecked(),
                           false, true, line, index))
            break;
        ed->replace(editReplace->text());
        ++count;
        ed->getCursorPosition(&line, &index);
    }
    ed->endUndoAction();
    main->statusBar()->showMessage(
        count ? QStringLiteral("%1 Treffer ersetzt.").arg(count)
              : QStringLiteral("Keine Treffer."), 4000);
}

// ---------------------------------------------------------------------------

// Command-line mode: npplx --to-pdf <input> [output] [--plain]
// Produces a PDF without a GUI; Markdown is rendered, everything else is
// treated as plain text. Force the offscreen platform unless already set.
static bool hasToPdfFlag(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (qstrcmp(argv[i], "--to-pdf") == 0)
            return true;
    }
    return false;
}

int main(int argc, char *argv[])
{
    const bool convertMode = hasToPdfFlag(argc, argv);
    if (convertMode
        && qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("NppLX"));
    QApplication::setOrganizationName(QStringLiteral("NppLX"));
    QApplication::setStyle(QStringLiteral("Fusion"));

    if (convertMode) {
        const QStringList args = QCoreApplication::arguments();
        QString inPath;
        QString outPath;
        bool plain = false;
        for (int i = 2; i < args.size(); ++i) {
            const QString a = args.at(i);
            if (a == QLatin1String("--plain"))
                plain = true;
            else if (inPath.isEmpty())
                inPath = a;
            else if (outPath.isEmpty())
                outPath = a;
        }
        if (inPath.isEmpty()) {
            QTextStream(stderr)
                << "Verwendung: npplx --to-pdf <Eingabedatei> [Ausgabe.pdf] "
                   "[--plain]\n"
                   "Markdown-Dateien werden gerendert; ohne Markdown-Endung "
                   "wird Klartext verwendet.\n";
            return 2;
        }
        if (outPath.isEmpty()) {
            outPath = QFileInfo(inPath).absolutePath() + QLatin1Char('/')
                      + QFileInfo(inPath).completeBaseName()
                      + QStringLiteral(".pdf");
        }
        return convertToPdfCli(inPath, outPath, plain);
    }

    MainWindow window;
    window.show();
    return QApplication::exec();
}
