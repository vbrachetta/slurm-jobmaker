/*
 *  Slurm JobMaker
 *  --------------
 *
 *  A lightweight graphical utility for composing Slurm job submission scripts.
 *  Parameters such as resources, runtime, partitions, accounts, and Quality of
 *  Service (QoS) are configured interactively. The live preview panel shows the
 *  exact script content as fields are filled in. Cluster-specific settings are
 *  loaded from plain-text profile files (.conf), making the tool portable
 *  across different HPC environments without recompilation.
 *
 *  Author:      Vincenzo Brachetta
 *  Institution: University of Birmingham
 *  Year:        2026
 *
 *  Licence:     MIT Licence — see the accompanying LICENCE file.
 *
 *  Disclaimer:  The software is provided "as is", without warranty of any kind.
 *               Users should verify that generated Slurm scripts conform to
 *               local HPC policies and queue requirements.
 */

#include <QApplication>
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QComboBox>
#include <QSpinBox>
#include <QKeyEvent>
#include <QPlainTextEdit>
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QGroupBox>
#include <QFileDialog>
#include <QSplitter>
#include <QFont>

// ---------------------------------------------------------------------------
// ProfileData
// ---------------------------------------------------------------------------
struct ProfileData {
    QString     name;
    QStringList qosOptions;
    QStringList constraintKeys;
    QStringList constraintLabels;
    QStringList preambleLines;
};

// ---------------------------------------------------------------------------
// loadProfile
// ---------------------------------------------------------------------------
ProfileData loadProfile(const QString &path)
{
    ProfileData p;
    p.name = "Generic";

    QFileInfo fi(path);
    if (!fi.exists() || !fi.isReadable()) {
        p.constraintKeys   = { "" };
        p.constraintLabels = { "No constraint" };
        return p;
    }

    QSettings s(path, QSettings::IniFormat);

    p.name = s.value("Profile/name", "Generic").toString();

    // QOS — guard against QSettings splitting on commas internally
    QVariant qosVariant = s.value("QOS/options", "");
    QString qosRaw;
    if (qosVariant.typeId() == QMetaType::QStringList)
        qosRaw = qosVariant.toStringList().join(",");
    else
        qosRaw = qosVariant.toString();
    p.qosOptions = qosRaw.split(',', Qt::SkipEmptyParts);
    for (auto &q : p.qosOptions) q = q.trimmed();

    // Constraints — same guard
    QVariant conVariant = s.value("Constraints/options", "");
    QString conRaw;
    if (conVariant.typeId() == QMetaType::QStringList)
        conRaw = conVariant.toStringList().join(",");
    else
        conRaw = conVariant.toString();

    QStringList conEntries = conRaw.split(',', Qt::SkipEmptyParts);
    for (auto &entry : conEntries) {
        entry = entry.trimmed();
        int colon = entry.indexOf(':');
        if (colon == -1) {
            p.constraintKeys.append(entry);
            p.constraintLabels.append(entry);
        } else {
            p.constraintKeys.append(entry.left(colon).trimmed());
            p.constraintLabels.append(entry.mid(colon + 1).trimmed());
        }
    }

    if (p.constraintKeys.isEmpty()) {
        p.constraintKeys.append("");
        p.constraintLabels.append("No constraint");
    }

    // Preamble — numbered keys: line1, line2, line3...
    int lineNum = 1;
    while (true) {
        QString key = QString("Preamble/line%1").arg(lineNum);
        if (!s.contains(key))
            break;
        QString val = s.value(key).toString().trimmed();
        if (!val.isEmpty())
            p.preambleLines.append(val);
        ++lineNum;
    }

    return p;
}

// ---------------------------------------------------------------------------
// buildScript — pure function, no GUI dependency
// ---------------------------------------------------------------------------
QString buildScript(const QString     &jobName,
                    const QString     &output,
                    int                ntasks,
                    int                cpus,
                    int                memGB,
                    const QString     &time,
                    const QString     &qos,
                    const QString     &account,
                    const QString     &constraintKey,
                    const QString     &mailType,
                    const QString     &mailUser,
                    const QStringList &preambleLines)
{
    QString script;
    QTextStream out(&script);

    out << "#!/bin/bash\n\n";
    out << "#SBATCH --job-name="      << jobName << "\n";

    // Output filename is optional — omitted if left blank
    if (!output.isEmpty())
        out << "#SBATCH --output=" << output << "\n";

    out << "#SBATCH --ntasks="        << ntasks  << "\n";
    out << "#SBATCH --cpus-per-task=" << cpus    << "\n";
    out << "#SBATCH --mem="           << memGB   << "G\n";
    out << "#SBATCH --time="          << time    << "\n";

    if (!qos.isEmpty() && qos != "No QoS")
        out << "#SBATCH --qos=" << qos << "\n";

    if (!account.isEmpty())
        out << "#SBATCH --account=" << account << "\n";

    if (!constraintKey.isEmpty())
        out << "#SBATCH --constraint=" << constraintKey << "\n";

    if (mailType != "None" && !mailType.isEmpty()) {
        out << "#SBATCH --mail-type=" << mailType << "\n";
        out << "#SBATCH --mail-user=" << mailUser << "\n";
    }

    out << "\nset -e\n";

    if (!preambleLines.isEmpty()) {
        out << "\n";
        for (const auto &line : preambleLines)
            out << line << "\n";
    }

    out << "\n# Your commands here\n";

    return script;
}

// ---------------------------------------------------------------------------
// MainWindow
// ---------------------------------------------------------------------------
class MainWindow : public QWidget {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setWindowTitle("Slurm JobMaker");
        setMinimumSize(800, 700);

        // ── Left panel — form ───────────────────────────────────────────────
        auto *formWidget = new QWidget;
        auto *formLayout = new QVBoxLayout(formWidget);
        formLayout->setSpacing(10);
        formLayout->setContentsMargins(10, 10, 6, 10);

        // ── Profile ──────────────────────────────────────────────────────────
        auto *profileGroup  = new QGroupBox("Profile");
        auto *profileLayout = new QHBoxLayout(profileGroup);

        profileLabel = new QLabel("No profile loaded.");
        auto *browseProfileButton = new QPushButton("Load profile...");

        profileLayout->addWidget(profileLabel, 1);
        profileLayout->addWidget(browseProfileButton);
        formLayout->addWidget(profileGroup);

        // ── Job Identity ─────────────────────────────────────────────────────
        auto *identityGroup  = new QGroupBox("Job Identity");
        auto *identityLayout = new QFormLayout(identityGroup);

        jobnameEdit = new QLineEdit("myjob");

        outputEdit = new QLineEdit("myjob.out");
        outputEdit->setPlaceholderText("Leave blank for default");

        identityLayout->addRow("Job name:",        jobnameEdit);
        identityLayout->addRow("Output filename:", outputEdit);
        formLayout->addWidget(identityGroup);

        // ── Resources ────────────────────────────────────────────────────────
        auto *resourcesGroup  = new QGroupBox("Resources");
        auto *resourcesLayout = new QFormLayout(resourcesGroup);

        tasksSpin = new QSpinBox;
        tasksSpin->setRange(1, 1024);
        tasksSpin->setValue(1);

        cpusSpin = new QSpinBox;
        cpusSpin->setRange(1, 128);
        cpusSpin->setValue(1);

        memorySpin = new QSpinBox;
        memorySpin->setRange(1, 4096);
        memorySpin->setValue(4);
        memorySpin->setSuffix(" GB");

        timeEdit = new QLineEdit("00-00:30:00");
        timeEdit->setValidator(new QRegularExpressionValidator(
            QRegularExpression(R"(\d{2}-\d{2}:\d{2}:\d{2})"), this));
        timeEdit->setPlaceholderText("DD-HH:MM:SS");
        timeEdit->setToolTip("Format: DD-HH:MM:SS  e.g. 00-02:30:00");

        resourcesLayout->addRow("Number of tasks:",    tasksSpin);
        resourcesLayout->addRow("CPUs per task:",      cpusSpin);
        resourcesLayout->addRow("Memory:",             memorySpin);
        resourcesLayout->addRow("Time (DD-HH:MM:SS):", timeEdit);
        formLayout->addWidget(resourcesGroup);

        // ── Cluster Settings ──────────────────────────────────────────────────
        auto *clusterGroup  = new QGroupBox("Cluster Settings");
        auto *clusterLayout = new QFormLayout(clusterGroup);

        qosDropdown = new QComboBox;
        qosDropdown->addItem("No QoS");
        constraintDropdown = new QComboBox;
        constraintDropdown->addItem("No constraint");

        accountEdit = new QLineEdit;
        accountEdit->setPlaceholderText("Enter account name");

        clusterLayout->addRow("Quality of Service:", qosDropdown);
        clusterLayout->addRow("CPU Constraint:",     constraintDropdown);
        clusterLayout->addRow("Account:",            accountEdit);
        formLayout->addWidget(clusterGroup);

        // ── Email Notifications ───────────────────────────────────────────────
        auto *notifGroup  = new QGroupBox("Email Notifications");
        auto *notifLayout = new QFormLayout(notifGroup);

        mailtypeDropdown = new QComboBox;
        mailtypeDropdown->addItems({ "None", "BEGIN", "END", "FAIL", "ALL" });

        emailEdit = new QLineEdit;
        emailEdit->setPlaceholderText("Enter your email address");

        notifLayout->addRow("Mail type:",    mailtypeDropdown);
        notifLayout->addRow("Mail address:", emailEdit);
        formLayout->addWidget(notifGroup);

        // ── Output File ───────────────────────────────────────────────────────
        auto *fileGroup  = new QGroupBox("Output File");
        auto *fileLayout = new QHBoxLayout(fileGroup);

        filenameEdit = new QLineEdit("script.sh");
        auto *browseButton = new QPushButton("Browse...");
        browseButton->setFixedWidth(80);
        fileLayout->addWidget(filenameEdit);
        fileLayout->addWidget(browseButton);
        formLayout->addWidget(fileGroup);

        formLayout->addStretch();

        // ── Right panel — live preview ────────────────────────────────────────
        auto *previewWidget = new QWidget;
        auto *previewLayout = new QVBoxLayout(previewWidget);
        previewLayout->setSpacing(6);
        previewLayout->setContentsMargins(6, 10, 10, 10);

        auto *previewTitle = new QLabel("<b>Live Script Preview</b>");
        previewLayout->addWidget(previewTitle);

        previewEdit = new QPlainTextEdit;
        previewEdit->setReadOnly(true);
        QFont mono("Monospace");
        mono.setStyleHint(QFont::TypeWriter);
        mono.setPointSize(14);
        previewEdit->setFont(mono);
        previewEdit->setStyleSheet(
            "QPlainTextEdit { background-color: #1e1e1e; color: #d4d4d4; "
            "border: 1px solid #555; border-radius: 4px; }");
        previewLayout->addWidget(previewEdit);

        // ── Buttons ───────────────────────────────────────────────────────────
        auto *saveButton   = new QPushButton("Save Script");
        auto *aboutButton  = new QPushButton("About");
        auto *cancelButton = new QPushButton("Cancel");

        auto *buttonLayout = new QHBoxLayout;
        buttonLayout->addWidget(saveButton);
        buttonLayout->addWidget(aboutButton);
        buttonLayout->addStretch();
        buttonLayout->addWidget(cancelButton);
        previewLayout->addLayout(buttonLayout);

        // ── Splitter ──────────────────────────────────────────────────────────
        auto *splitter = new QSplitter(Qt::Horizontal, this);
        splitter->addWidget(formWidget);
        splitter->addWidget(previewWidget);
        splitter->setStretchFactor(0, 2);
        splitter->setStretchFactor(1, 3);
        splitter->setHandleWidth(6);

        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->addWidget(splitter);

        // ── Signals ───────────────────────────────────────────────────────────
        connect(jobnameEdit,        &QLineEdit::textChanged,
                this, &MainWindow::updatePreview);
        connect(outputEdit,         &QLineEdit::textChanged,
                this, &MainWindow::updatePreview);
        connect(timeEdit,           &QLineEdit::textChanged,
                this, &MainWindow::updatePreview);
        connect(accountEdit,        &QLineEdit::textChanged,
                this, &MainWindow::updatePreview);
        connect(emailEdit,          &QLineEdit::textChanged,
                this, &MainWindow::updatePreview);
        connect(filenameEdit,       &QLineEdit::textChanged,
                this, &MainWindow::updatePreview);
        connect(tasksSpin,          QOverload<int>::of(&QSpinBox::valueChanged),
                this, &MainWindow::updatePreview);
        connect(cpusSpin,           QOverload<int>::of(&QSpinBox::valueChanged),
                this, &MainWindow::updatePreview);
        connect(memorySpin,         QOverload<int>::of(&QSpinBox::valueChanged),
                this, &MainWindow::updatePreview);
        connect(qosDropdown,        QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &MainWindow::updatePreview);
        connect(constraintDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &MainWindow::updatePreview);
        connect(mailtypeDropdown,   QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &MainWindow::updatePreview);

        connect(browseProfileButton, &QPushButton::clicked,
                this, &MainWindow::browseProfile);
        connect(browseButton,        &QPushButton::clicked,
                this, &MainWindow::browseOutputFile);
        connect(saveButton,          &QPushButton::clicked,
                this, &MainWindow::saveScript);
        connect(aboutButton,         &QPushButton::clicked,
                this, &MainWindow::showAbout);
        connect(cancelButton,        &QPushButton::clicked,
                this, &QWidget::close);

        updatePreview();
    }

protected:
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Escape)
            close();
        else
            QWidget::keyPressEvent(event);
    }

private slots:

    void browseProfile() {
        QString path = QFileDialog::getOpenFileName(
            this, "Open Profile",
            QDir::homePath(),
            "Profile files (*.conf);;All files (*)");
        if (path.isEmpty())
            return;

        ProfileData newProfile = loadProfile(path);

        // Warn if the file could not be read correctly
        if (newProfile.qosOptions.isEmpty() &&
            newProfile.constraintLabels == QStringList{"No constraint"} &&
            newProfile.preambleLines.isEmpty() &&
            newProfile.name == "Generic") {
            QMessageBox::warning(this, "Profile Warning",
                QString("The file could not be read or appears to be empty:\n%1\n\n"
                        "A generic fallback profile has been loaded.").arg(path));
        }

        applyProfile(newProfile);
    }

    void applyProfile(const ProfileData &newProfile) {
        m_profile = newProfile;

        setWindowTitle(QString("Slurm JobMaker — %1").arg(newProfile.name));
        profileLabel->setText(
            QString("Active profile: <b>%1</b>").arg(newProfile.name));

        qosDropdown->clear();
        qosDropdown->addItem("No QoS");
        for (const auto &q : newProfile.qosOptions)
            qosDropdown->addItem(q);

        constraintDropdown->clear();
        for (int i = 0; i < newProfile.constraintLabels.size(); ++i)
            constraintDropdown->addItem(newProfile.constraintLabels.at(i));

        updatePreview();
    }

    void updatePreview() {
        int constraintIdx = constraintDropdown->currentIndex();
        QString constraintKey;
        if (constraintIdx >= 0 && constraintIdx < m_profile.constraintKeys.size())
            constraintKey = m_profile.constraintKeys.at(constraintIdx);

        QString script = buildScript(
            jobnameEdit->text().trimmed(),
            outputEdit->text().trimmed(),
            tasksSpin->value(),
            cpusSpin->value(),
            memorySpin->value(),
            timeEdit->text().trimmed(),
            qosDropdown->currentText(),
            accountEdit->text().trimmed(),
            constraintKey,
            mailtypeDropdown->currentText(),
            emailEdit->text().trimmed(),
            m_profile.preambleLines
        );

        previewEdit->setPlainText(script);
    }

    void browseOutputFile() {
        QString path = QFileDialog::getSaveFileName(
            this, "Save Slurm script",
            QDir::homePath(),
            "Shell scripts (*.sh);;All files (*)");
        if (!path.isEmpty())
            filenameEdit->setText(path);
    }

    void saveScript() {
        if (jobnameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Validation", "Job name cannot be empty.");
            jobnameEdit->setFocus();
            return;
        }
        if (filenameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Validation", "Script filename cannot be empty.");
            filenameEdit->setFocus();
            return;
        }

        static const QRegularExpression timeRx(R"(^\d{2}-\d{2}:\d{2}:\d{2}$)");
        if (!timeRx.match(timeEdit->text()).hasMatch()) {
            QMessageBox::warning(this, "Validation",
                "Time must be in DD-HH:MM:SS format (e.g. 00-02:30:00).");
            timeEdit->setFocus();
            return;
        }

        if (mailtypeDropdown->currentText() != "None" &&
            emailEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Validation",
                "Please enter an email address, or set Mail type to None.");
            emailEdit->setFocus();
            return;
        }

        QFile file(filenameEdit->text().trimmed());
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "Error",
                QString("Cannot open file for writing:\n%1").arg(file.errorString()));
            return;
        }
        QTextStream(&file) << previewEdit->toPlainText();
        file.close();

        QMessageBox::information(this, "Saved",
            QString("Script saved to:\n%1").arg(filenameEdit->text().trimmed()));
    }

    void showAbout() {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("About Slurm JobMaker");
        msgBox.setTextFormat(Qt::RichText);
        msgBox.setTextInteractionFlags(Qt::TextBrowserInteraction);
        msgBox.setText(
            "<b>Slurm JobMaker</b><br>"
            "Version 1.0.0<br><br>"
            "An open-source, portable graphical tool for generating Slurm job "
            "submission scripts. Supports multiple HPC clusters via plain-text "
            "profile files. Designed for usability and reproducibility in "
            "research computing environments.<br><br>"
            "© 2026 Vincenzo Brachetta — MIT Licence<br><br>"
            "<a href='https://github.com/vbrachetta/slurm-jobmaker'>"
            "GitHub Repository</a>"
            "  |  "
            "<a href='https://doi.org/10.5281/zenodo.19629545'>"
            "Zenodo Record</a>"
        );
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    }

private:
    ProfileData  m_profile;

    QLabel         *profileLabel;

    QLineEdit      *filenameEdit;
    QLineEdit      *jobnameEdit;
    QLineEdit      *outputEdit;
    QLineEdit      *timeEdit;
    QLineEdit      *accountEdit;
    QLineEdit      *emailEdit;

    QSpinBox       *tasksSpin;
    QSpinBox       *cpusSpin;
    QSpinBox       *memorySpin;

    QComboBox      *qosDropdown;
    QComboBox      *constraintDropdown;
    QComboBox      *mailtypeDropdown;

    QPlainTextEdit *previewEdit;
};

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.show();
    return app.exec();
}

#include "main.moc"
