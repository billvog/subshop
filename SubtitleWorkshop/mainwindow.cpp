#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    SetupButtonIcons();
    SetupVideoWidget();
    SetupSubtitlesTable();
    ConnectEvents();

    // Media Player Group
    ui->TogglePlayButton->setEnabled(false);
    ui->BackwardSeekButton->setEnabled(false);
    ui->ForwardSeekButton->setEnabled(false);
    ui->StopButton->setEnabled(false);

    // Subtitle Group
    ui->SubtitleGroupBox->setEnabled(false);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *e) {
    if (!CheckIfSaved()) {
        e->ignore();
        return;
    }

    e->accept();
}

void MainWindow::resizeEvent(QResizeEvent *) {
    UpdateUI();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e) {
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *e) {
    const QMimeData *mimeData = e->mimeData();

    if (mimeData->hasUrls()) {
        QString path(mimeData->urls().at(0).toLocalFile());

        QFileInfo fileInfo(path);
        QString suffix(fileInfo.suffix());

        // Uhuu Buhuu:: Make things do stuff no one else can get done.
        if (SubtitleFileSelector.contains(suffix)) {
            OpenSubtitleFile(path);
        }
        else if (MediaFileSelector.contains(suffix)) {
            OpenMediaFile(path);
        }
        else {
            QMessageBox::critical(this, "Error", "Unsupported file type \"" + suffix + "\"");
            return;
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Escape) {
        ui->SubtitleTextEdit->clearFocus();
    }
}

void MainWindow::SetupButtonIcons() {
    ui->TogglePlayButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->StopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    ui->ToggleMuteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));

    ui->BackwardSeekButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward));
    ui->ForwardSeekButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));

    ui->PrevSubButton->setIcon(style()->standardIcon(QStyle::SP_ArrowLeft));
    ui->NextSubButton->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));

    ui->ApplySubButton->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    ui->RemoveSubButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
}

void MainWindow::SetupVideoWidget() {
    videoItem = new QGraphicsVideoItem();
    subTextItem = new QGraphicsTextItem();

    scene = new QGraphicsScene(this);

    QGraphicsView *view = ui->GraphicsView;
    view->setScene(scene);
    view->show();

    scene->addItem(videoItem);
    scene->addItem(subTextItem);

    player = new QMediaPlayer(this, QMediaPlayer::VideoSurface);
    player->setVideoOutput(videoItem);
    player->setNotifyInterval(50);
    player->setVolume(ui->VolumeSlider->value());

    subTextItem->setPlainText(QString());
    subTextItem->setDefaultTextColor(QColorConstants::White);

    QFont subtitleFont = subTextItem->font();
    subtitleFont.setPixelSize(26);
    subTextItem->setFont(subtitleFont);

    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setOffset(1, 1);
    subTextItem->setGraphicsEffect(shadowEffect);
}

void MainWindow::SetupSubtitlesTable() {
    subtitlesModel = new QStandardItemModel(Subtitles.size(), 3, this);
    subtitlesModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Show")));
    subtitlesModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Hide")));
    subtitlesModel->setHorizontalHeaderItem(2, new QStandardItem(QString("Subtitle")));

    ui->SubTableView->setModel(subtitlesModel);
}

void MainWindow::ConnectEvents() {
    // File Menu
    connect(ui->ActionNew, SIGNAL(triggered()), this, SLOT(NewAction()));
    connect(ui->ActionOpen, SIGNAL(triggered()), this, SLOT(OpenAction()));
    connect(ui->ActionSave, SIGNAL(triggered()), this, SLOT(SaveAction()));
    connect(ui->ActionSaveAs, SIGNAL(triggered()), this, SLOT(SaveAsAction()));
    connect(ui->ActionClose, SIGNAL(triggered()), this, SLOT(CloseAction()));
    connect(ui->ActionExit, SIGNAL(triggered()), this, SLOT(ExitAction()));

    // Edit Menu
    connect(ui->ActionEditUndo, SIGNAL(triggered()), this, SLOT(UndoAction()));
    connect(ui->ActionEditRedo, SIGNAL(triggered()), this, SLOT(RedoAction()));

    // Media Menu
    connect(ui->ActionMediaOpen, SIGNAL(triggered()), this, SLOT(OpenMediaAction()));
    connect(ui->ActionMediaClose, SIGNAL(triggered()), this, SLOT(CloseMediaAction()));
    connect(ui->ActionMediaPlayPause, SIGNAL(triggered()), this, SLOT(TogglePlayVideo()));
    connect(ui->ActionMediaStop, SIGNAL(triggered()), this, SLOT(StopVideo()));
    connect(ui->ActionMediaSeekBackward, SIGNAL(triggered()), this, SLOT(SeekBackwards()));
    connect(ui->ActionMediaSeekForward, SIGNAL(triggered()), this, SLOT(SeekForwards()));
    connect(ui->ActionMediaAudioVolumeUp, SIGNAL(triggered()), this, SLOT(VolumeUp()));
    connect(ui->ActionMediaAudioVolumeDown, SIGNAL(triggered()), this, SLOT(VolumeDown()));
    connect(ui->ActionMediaAudioToggleMute, SIGNAL(triggered()), this, SLOT(ToggleMuteAudio()));

    // Subtitle Menu
    connect(ui->ActionSubGotoPrevious, SIGNAL(triggered()), this, SLOT(GotoPreviousSub()));
    connect(ui->ActionSubGotoNext, SIGNAL(triggered()), this, SLOT(GotoNextSub()));

    // Help Menu
    connect(ui->ActionHelpAbout, SIGNAL(triggered()), this, SLOT(AboutHelpAction()));

    // Media Player
    connect(player, SIGNAL(seekableChanged(bool)), this, SLOT(VideoSeekableChanged(bool)));
    connect(player, SIGNAL(positionChanged(qint64)), this, SLOT(VideoPositionChanged(qint64)));
    connect(player, SIGNAL(durationChanged(qint64)), this, SLOT(VideoDurationChanged(qint64)));

    connect(ui->TimelineSlider, SIGNAL(sliderMoved(int)), this, SLOT(TimelineSliderChanged(int)));
    connect(ui->TogglePlayButton, SIGNAL(clicked()), this, SLOT(TogglePlayVideo()));
    connect(ui->StopButton, SIGNAL(clicked()), this, SLOT(StopVideo()));

    connect(ui->BackwardSeekButton, SIGNAL(clicked()), this, SLOT(SeekBackwards()));
    connect(ui->ForwardSeekButton, SIGNAL(clicked()), this, SLOT(SeekForwards()));

    connect(ui->ToggleMuteButton, SIGNAL(clicked()), this, SLOT(ToggleMuteAudio()));
    connect(ui->VolumeSlider, SIGNAL(sliderMoved(int)), this, SLOT(VolumeSliderChanged(int)));

    // Subtitle Group
    connect(ui->SubTableView, SIGNAL(clicked(QModelIndex)), this, SLOT(SubTableRowClicked(QModelIndex)));

    connect(ui->PrevSubButton, SIGNAL(clicked()), this, SLOT(GotoPreviousSub()));
    connect(ui->NextSubButton, SIGNAL(clicked()), this, SLOT(GotoNextSub()));

    connect(ui->ShowSubTimeEdit, SIGNAL(editingFinished()), this, SLOT(SubShowTimeChanged()));
    connect(ui->HideSubTimeEdit, SIGNAL(editingFinished()), this, SLOT(SubHideTimeChanged()));
    connect(ui->DurationSubTimeEdit, SIGNAL(editingFinished()), this, SLOT(SubDurationChanged()));

    connect(ui->SubtitleTextEdit, SIGNAL(textChanged()), this, SLOT(SubTextChanged()));
    connect(ui->SubtitleTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(SubCursorPosChanged()));

    connect(ui->SubBoldButton,  SIGNAL(clicked()), this, SLOT(SubBoldClicked()));
    connect(ui->SubItalicButton,  SIGNAL(clicked()), this, SLOT(SubItalicClicked()));
    connect(ui->SubUnderlineButton,  SIGNAL(clicked()), this, SLOT(SubUnderlineClicked()));
    connect(ui->SubStrikeoutButton,  SIGNAL(clicked()), this, SLOT(SubStrikeoutClicked()));

    connect(ui->ApplySubButton, SIGNAL(clicked()), this, SLOT(ApplySubtitle()));
    connect(ui->RemoveSubButton, SIGNAL(clicked()), this, SLOT(RemoveSubtitle()));
}

void MainWindow::UpdateUI() {
    videoItem->setSize(ui->GraphicsView->size());
    scene->setSceneRect(0, 0, videoItem->size().width(), videoItem->size().height());

    // Update Subtitle Text
    subTextScaleFactor = std::clamp(scene->itemsBoundingRect().width() / 622, 0.0, 1.0);
    subTextItem->setScale(subTextScaleFactor);

    UpdateSubPosition();
}

void MainWindow::UpdateSubPosition() {
    QSizeF textRectSize = subTextItem->boundingRect().size() * subTextScaleFactor;
    qreal target_y = videoItem->size().height() - textRectSize.height();
    qreal target_x = (videoItem->size().width() - textRectSize.width()) / 2;
    subTextItem->setPos(target_x, target_y);
}

QTime MainWindow::MsToTime(int ms) {
    if (ms < 0)
        return QTime();

    int hours, minutes, seconds, milliseconds = ms;

    if (milliseconds < 3600000)
        hours = 0;
    else {
        hours = milliseconds / 3600000;
        milliseconds -= hours * 3600000;
    }

    if (milliseconds < 60000)
        minutes = 0;
    else {
        minutes = milliseconds / 60000;
        milliseconds -= minutes * 60000;
    }

    if (milliseconds < 1000)
        seconds = 0;
    else {
        seconds = milliseconds / 1000;
        milliseconds -= seconds * 1000;
    }

    return QTime(hours, minutes, seconds, milliseconds);
}

bool MainWindow::CheckIfSaved() {
    if (hasFileOpen) {
        if (!isSaved) {
            int result = QMessageBox::question(this, "Confirm", "File \"" + QFileInfo(SubFilePath).fileName() + "\" has changed.\nDo you want to save changes?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);
            if (result == QMessageBox::Yes) {
                SaveAction();
            }
            else if (result == QMessageBox::Cancel) {
                return false;
            }
        }
    }

    return true;
}

void MainWindow::SetIsSaved(bool value) {
    QString windowTitlePrefix;

    QString filename = QFileInfo(SubFilePath).fileName();
    if (filename.isEmpty())
        filename = "untitled";

    if (value) {
        windowTitlePrefix = filename;
    }
    else {
        windowTitlePrefix = "*" + filename;
    }

    setWindowTitle(windowTitlePrefix + " - Subshop");
    hasFileOpen = true;
    isSaved = value;
}

// Actions
// File
void MainWindow::NewAction() {
    if (!CheckIfSaved()) {
        return;
    }

    subtitlesModel->clear();
    Subtitles.clear();

    SubFilePath.clear();
    setWindowTitle("untitled - Subshop");

    UndoItems.clear();
    RedoItems.clear();

    EditingSubtitleIndex = -1;
    PrevEditinSubtitleIndex = EditingSubtitleIndex;

    subTextItem->setPlainText(QString());

    ui->SubtitleTextEdit->setPlainText(QString());
    ui->ShowSubTimeEdit->setTime(QTime());
    ui->HideSubTimeEdit->setTime(QTime());

    ui->SubtitleGroupBox->setEnabled(true);
    SetIsSaved(false);
}

void MainWindow::OpenAction() {
    if (!CheckIfSaved()) return;

    QString file = QFileDialog::getOpenFileName(this, "Open Subtitle File", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), SubtitleFileSelector);

    if (file.isEmpty()) return;

    if (!QFile(file).exists()) {
        QMessageBox::critical(this, "Error", "File \"" + file + "\" doesn't exist");
        return;
    }

    OpenSubtitleFile(file);
}

void MainWindow::SaveAction() {
    if (SubFilePath.isEmpty()) {
        SaveAsAction();
        return;
    }

    QString suffix(QFileInfo(SubFilePath).suffix());
    if (suffix == "srt") {
        if (!SubParser::ExportSrt(Subtitles, SubFilePath)) {
            QMessageBox::critical(this, "Error", "Could't save subtitle file to \"" + SubFilePath + "\"");
            return;
        }
    }
    else if (suffix == "vtt") {
        if (!SubParser::ExportVtt(Subtitles, SubFilePath)) {
            QMessageBox::critical(this, "Error", "Could't save subtitle file to \"" + SubFilePath + "\"");
            return;
        }
    }
    else {
        QMessageBox::critical(this, "Error", "Unsupported file type \"" + suffix + "\"");
        return;
    }

    SetIsSaved(true);
}

void MainWindow::SaveAsAction() {
    QString file = QFileDialog::getSaveFileName(this, "Save Subtitle File As", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), SubtitleFileSelector);

    if (file.isEmpty()) {
        return;
    }

    QString suffix(QFileInfo(file).suffix());
    if (suffix == "srt") {
        if (!SubParser::ExportSrt(Subtitles, file)) {
            QMessageBox::critical(this, "Error", "Could't save subtitle file to \"" + file + "\"");
            return;
        }
    }
    else if (suffix == "vtt") {
        if (!SubParser::ExportVtt(Subtitles, file)) {
            QMessageBox::critical(this, "Error", "Could't save subtitle file to \"" + file + "\"");
            return;
        }
    }
    else {
        QMessageBox::critical(this, "Error", "Unsupported file type \"" + suffix + "\"");
        return;
    }

    SubFilePath = file;
    QFileInfo fileInfo(SubFilePath);
    setWindowTitle(fileInfo.fileName() + " - Subshop");

    SetIsSaved(true);
}

void MainWindow::CloseAction() {
    if (!CheckIfSaved()) {
        return;
    }

    subtitlesModel->clear();
    Subtitles.clear();

    SubFilePath.clear();
    setWindowTitle("Subshop");

    EditingSubtitleIndex = -1;
    PrevEditinSubtitleIndex = EditingSubtitleIndex;

    UndoItems.clear();
    RedoItems.clear();

    subTextItem->setPlainText(QString());

    ui->SubtitleTextEdit->setPlainText(QString());
    ui->ShowSubTimeEdit->setTime(QTime());
    ui->HideSubTimeEdit->setTime(QTime());

    ui->SubtitleGroupBox->setEnabled(false);
    hasFileOpen = false;
}

void MainWindow::ExitAction() {
    this->close();
}

// Edit
void MainWindow::UndoAction() {
    if (UndoItems.isEmpty()) {
        return;
    }

    SubtitleItem NewItem;
    UndoItem undo = UndoItems.last();
    UndoItem::ItemType itemType = undo.getItemType();

    if (itemType == UndoItem::ItemType::ADD) {
        int i = Subtitles.indexOf(undo.getNewItem());
        if (i >= 0) {
            Subtitles.removeAt(i);
            subtitlesModel->removeRow(i);
        }
    }
    else if (itemType == UndoItem::ItemType::REMOVE) {
        SubtitleItem SubItem = undo.getNewItem();
        if (Subtitles.indexOf(SubItem) != -1) {
            RedoItems.removeLast();
            return;
        }

        subtitlesModel->setItem(Subtitles.size(), 0, new QStandardItem(SubItem.getShowTimestamp().toString("hh:mm:ss,zzz")));
        subtitlesModel->setItem(Subtitles.size(), 1, new QStandardItem(SubItem.getHideTimestamp().toString("hh:mm:ss,zzz")));
        subtitlesModel->setItem(Subtitles.size(), 2, new QStandardItem(SubItem.getSubtitle()));

        Subtitles.push_back(SubItem);

        NewItem = SubItem;
    }
    else if (itemType == UndoItem::ItemType::EDIT) {
        int i = Subtitles.indexOf(undo.getNewItem());
        if (i >= 0) {
            SubtitleItem SubItem(undo.getOldItem());

            subtitlesModel->setItem(i, 0, new QStandardItem(SubItem.getShowTimestamp().toString("hh:mm:ss,zzz")));
            subtitlesModel->setItem(i, 1, new QStandardItem(SubItem.getHideTimestamp().toString("hh:mm:ss,zzz")));
            subtitlesModel->setItem(i, 2, new QStandardItem(SubItem.getSubtitle()));

            Subtitles.replace(i, SubItem);

            NewItem = SubItem;
        }
    }

    RedoItems.append(undo);
    UndoItems.removeLast();

    subtitlesModel->sort(0);
    std::sort(Subtitles.begin(), Subtitles.end(), SubtitleItem::SortByShowTime);

    SelectSubFromTable(Subtitles.indexOf(NewItem));

    SetIsSaved(false);

    ShowAvailableSub();
}

void MainWindow::RedoAction() {
    if (RedoItems.isEmpty()) {
        return;
    }

    SubtitleItem NewItem;
    UndoItem redo = RedoItems.last();
    UndoItem::ItemType itemType = redo.getItemType();

    if (itemType == UndoItem::ItemType::ADD) {
        SubtitleItem SubItem = redo.getNewItem();
        if (Subtitles.indexOf(SubItem) != -1) {
            RedoItems.removeLast();
            return;
        }

        subtitlesModel->setItem(Subtitles.size(), 0, new QStandardItem(SubItem.getShowTimestamp().toString("hh:mm:ss,zzz")));
        subtitlesModel->setItem(Subtitles.size(), 1, new QStandardItem(SubItem.getHideTimestamp().toString("hh:mm:ss,zzz")));
        subtitlesModel->setItem(Subtitles.size(), 2, new QStandardItem(SubItem.getSubtitle()));

        Subtitles.push_back(SubItem);

        NewItem = SubItem;
    }
    else if (itemType == UndoItem::ItemType::REMOVE) {
        int i = Subtitles.indexOf(redo.getNewItem());
        if (i >= 0) {
            Subtitles.removeAt(i);
            subtitlesModel->removeRow(i);
        }
    }
    else if (itemType == UndoItem::ItemType::EDIT) {
        int i = Subtitles.indexOf(redo.getOldItem());
        if (i >= 0) {
            SubtitleItem SubItem(redo.getNewItem());

            subtitlesModel->setItem(i, 0, new QStandardItem(SubItem.getShowTimestamp().toString("hh:mm:ss,zzz")));
            subtitlesModel->setItem(i, 1, new QStandardItem(SubItem.getHideTimestamp().toString("hh:mm:ss,zzz")));
            subtitlesModel->setItem(i, 2, new QStandardItem(SubItem.getSubtitle()));

            Subtitles.replace(i, SubItem);

            NewItem = SubItem;
        }
    }

    UndoItems.append(redo);
    RedoItems.removeLast();

    subtitlesModel->sort(0);
    std::sort(Subtitles.begin(), Subtitles.end(), SubtitleItem::SortByShowTime);

    SelectSubFromTable(Subtitles.indexOf(NewItem));

    SetIsSaved(false);

    ShowAvailableSub();
}

// Media
void MainWindow::OpenMediaAction() {
    QString file = QFileDialog::getOpenFileName(this, "Open Movie", QStandardPaths::writableLocation(QStandardPaths::MoviesLocation), MediaFileSelector);

    if (file.isEmpty()) {
        return;
    }

    if (!QFile(file).exists()) {
        QMessageBox::critical(this, "Error", "File \"" + file + "\" doesn't exist");
        return;
    }

    OpenMediaFile(file);
}

void MainWindow::CloseMediaAction() {
    player->setMedia(QMediaContent());
    player->stop();

    ui->TogglePlayButton->setEnabled(false);
    ui->BackwardSeekButton->setEnabled(false);
    ui->ForwardSeekButton->setEnabled(false);
    ui->StopButton->setEnabled(false);

    ui->TogglePlayButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
}

// Help
void MainWindow::AboutHelpAction() {
    // Uhuu Buhuu:: make about dialog
    AboutDialog *dialog = new AboutDialog(this);
    dialog->show();
}

// Media player
void MainWindow::OpenMediaFile(const QString &Path) {
    player->setMedia(QUrl::fromLocalFile(Path));
    player->play();

    ui->TogglePlayButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    ui->StopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));

    ui->TogglePlayButton->setEnabled(true);
    ui->BackwardSeekButton->setEnabled(true);
    ui->ForwardSeekButton->setEnabled(true);
    ui->StopButton->setEnabled(true);
}

void MainWindow::VideoSeekableChanged(bool) {
    UpdateUI();
}

void MainWindow::VideoDurationChanged(qint64 value) {
    ui->TimelineSlider->setMaximum(value);
}

void MainWindow::VideoPositionChanged(qint64 value) {
    int TotalDuration = player->duration();
    int CurrentPosition = value;

    if (!ui->TimelineSlider->isSliderDown())
        ui->TimelineSlider->setValue(value);

    int SubDuration = QTime(0, 0, 0).msecsTo(ui->DurationSubTimeEdit->time());

    ui->ShowSubTimeEdit->setTime(MsToTime(CurrentPosition));
    ui->HideSubTimeEdit->setTime(MsToTime(CurrentPosition + SubDuration));

    ui->TimelineLabel->setText(MsToTime(CurrentPosition).toString("hh:mm:ss,zzz") + " / " + MsToTime(TotalDuration).toString("hh:mm:ss,zzz"));

    ShowAvailableSub();
}

void MainWindow::TimelineSliderChanged(int value) {
    player->setPosition(value);
}

void MainWindow::TogglePlayVideo() {
    if (player->mediaStatus() == QMediaPlayer::NoMedia)
        return;

    if (player->state() == QMediaPlayer::PlayingState) {
        ui->TogglePlayButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        player->pause();
    }
    else {
        ui->StopButton->setEnabled(true);
        ui->TogglePlayButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
        player->play();
    }
}

void MainWindow::StopVideo() {
    ui->TogglePlayButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->StopButton->setEnabled(false);

    player->stop();
}

void MainWindow::SeekForwards() {
    if (player->mediaStatus() == QMediaPlayer::NoMedia)
        return;

    player->setPosition(player->position() + 500);
}

void MainWindow::SeekBackwards() {
    if (player->mediaStatus() == QMediaPlayer::NoMedia)
        return;

    player->setPosition(player->position() - 500);
}

void MainWindow::VolumeUp() {
    ui->VolumeSlider->setValue(ui->VolumeSlider->value() + 5);
}

void MainWindow::VolumeDown() {
    ui->VolumeSlider->setValue(ui->VolumeSlider->value() - 5);
}

void MainWindow::ToggleMuteAudio() {
    if (player->isMuted()) {
        player->setMuted(false);
        ui->ToggleMuteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
    }
    else {
        player->setMuted(true);
        ui->ToggleMuteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolumeMuted));
    }
}

void MainWindow::VolumeSliderChanged(int value) {
    player->setVolume(value);
}

// Subtitle Group
void MainWindow::OpenSubtitleFile(const QString &Path) {
    SubFilePath = Path;
    QFileInfo fileInfo(SubFilePath);
    setWindowTitle(fileInfo.fileName() + " - Subshop");

    UndoItems.clear();
    RedoItems.clear();

    EditingSubtitleIndex = -1;
    PrevEditinSubtitleIndex = EditingSubtitleIndex;

    QString suffix(fileInfo.suffix());
    if (suffix == "srt") {
        Subtitles = SubParser::ParseSrt(SubFilePath);
    }
    else if (suffix == "vtt") {
        Subtitles = SubParser::ParseVtt(SubFilePath);
    }
    else {
        QMessageBox::critical(this, "Error", "Unsupported file type \"" + suffix + "\"");
        CloseAction();
        return;
    }

    subtitlesModel->clear();
    for (int i = 0; i < Subtitles.size(); i++) {
        subtitlesModel->setItem(i, 0, new QStandardItem(Subtitles.at(i).getShowTimestamp().toString("hh:mm:ss,zzz")));
        subtitlesModel->setItem(i, 1, new QStandardItem(Subtitles.at(i).getHideTimestamp().toString("hh:mm:ss,zzz")));
        subtitlesModel->setItem(i, 2, new QStandardItem(Subtitles.at(i).getSubtitle()));
    }

    subtitlesModel->sort(0);

    ui->SubtitleGroupBox->setEnabled(true);
    SetIsSaved(true);
}

void MainWindow::ShowAvailableSub() {
    int Position = player->position();

    ClearSubtitle();

    for (int i = 0; i < Subtitles.size(); i++) {
        int SubShowTime = QTime(0, 0, 0).msecsTo(Subtitles.at(i).getShowTimestamp());
        int SubHideTime = QTime(0, 0, 0).msecsTo(Subtitles.at(i).getHideTimestamp());

        int NextSubShowTime = -1;
        if (Subtitles.size()-1 >= i+1) {
            NextSubShowTime = QTime(0, 0, 0).msecsTo(Subtitles.at(i+1).getShowTimestamp());
        }

        if (SubShowTime <= Position && Position <= SubHideTime && (Position != NextSubShowTime)) {
            DisplaySubtitle(Subtitles.at(i));
            break;
        }
    }
}

void MainWindow::DisplaySubtitle(const SubtitleItem &subItem) {
    int index = Subtitles.indexOf(subItem);
    if (index == -1)
        return;

    // Display Subtitle on Video
    subTextItem->setHtml(subItem.getSubtitle().replace('\n', "<br>"));
    UpdateSubPosition();

    // Fill active Subtitle values on fields
    int SubShowTime = QTime(0, 0, 0).msecsTo(subItem.getShowTimestamp());
    int SubHideTime = QTime(0, 0, 0).msecsTo(subItem.getHideTimestamp());

    ui->ShowSubTimeEdit->setTime(subItem.getShowTimestamp());
    ui->HideSubTimeEdit->setTime(subItem.getHideTimestamp());
    ui->SubtitleTextEdit->setPlainText(subItem.getSubtitle());
    ui->DurationSubTimeEdit->setTime(MsToTime(SubHideTime - SubShowTime));

    // Select active Subtitle on table
    ui->SubTableView->selectRow(index);
    EditingSubtitleIndex = index;
    PrevEditinSubtitleIndex = EditingSubtitleIndex;

    isSubApplied = true;
}

void MainWindow::ClearSubtitle() {
    subTextItem->setPlainText(QString());

    ui->ShowSubTimeEdit->setTime(QTime());
    ui->HideSubTimeEdit->setTime(QTime());
    ui->SubtitleTextEdit->setPlainText(QString());

    ui->SubTableView->clearSelection();

    EditingSubtitleIndex = -1;
    isSubApplied = true;
}

void MainWindow::SelectSubFromTable(int row) {
    if (0 > row || row >= Subtitles.size())
        return;

    if (!isSubApplied && EditingSubtitleIndex >= 0) {
        SubtitleItem currentSub = Subtitles.at(EditingSubtitleIndex);
        QString Timestamps = currentSub.getShowTimestamp().toString("hh:mm:ss,zzz") + " - " + currentSub.getHideTimestamp().toString("hh:mm:ss,zzz");

        int result = QMessageBox::question(this, "Confirm", "Subtitle at \"" + Timestamps + "\" has changed. Apply?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);
        if (result == QMessageBox::Yes) {
            ApplySubtitle();
        }
        else if (result == QMessageBox::Cancel) {
            ui->SubTableView->selectRow(PrevEditinSubtitleIndex);
            return;
        }
    }

    player->setPosition(QTime(0, 0, 0).msecsTo(Subtitles.at(row).getShowTimestamp()));
}

void MainWindow::SubTableRowClicked(QModelIndex index) {
    SelectSubFromTable(index.row());
}

void MainWindow::GotoPreviousSub() {
    SelectSubFromTable(PrevEditinSubtitleIndex - 1);
}

void MainWindow::GotoNextSub() {
    SelectSubFromTable(PrevEditinSubtitleIndex + 1);
}

void MainWindow::SubShowTimeChanged() {
    int SubShowTime = QTime(0, 0, 0).msecsTo(ui->ShowSubTimeEdit->time());
    int SubHideTime = QTime(0, 0, 0).msecsTo(ui->HideSubTimeEdit->time());

    if (SubHideTime - SubShowTime < 0) {
        int SubDuration = QTime(0, 0, 0).msecsTo(ui->DurationSubTimeEdit->time());
        ui->HideSubTimeEdit->setTime(MsToTime(SubShowTime + SubDuration));
    }
}

void MainWindow::SubHideTimeChanged() {
    int SubShowTime = QTime(0, 0, 0).msecsTo(ui->ShowSubTimeEdit->time());
    int SubHideTime = QTime(0, 0, 0).msecsTo(ui->HideSubTimeEdit->time());

    ui->DurationSubTimeEdit->setTime(MsToTime(SubHideTime - SubShowTime));
}

void MainWindow::SubDurationChanged() {
    int SubShowTime = QTime(0, 0, 0).msecsTo(ui->ShowSubTimeEdit->time());
    int SubDuration = QTime(0, 0, 0).msecsTo(ui->DurationSubTimeEdit->time());

    ui->HideSubTimeEdit->setTime(MsToTime(SubShowTime + SubDuration));
}

void MainWindow::SubTextToggleTag(const QString &tag) {
    QTextCursor textCursor = ui->SubtitleTextEdit->textCursor();

    if (!textCursor.hasSelection()) {
        return;
    }

    QString selectedText(textCursor.selectedText());
    QString regExp("<" + tag + ">(.*)</" + tag + ">");

    if (QRegExp(regExp).exactMatch(selectedText)) {
        selectedText.replace(QRegExp(regExp), "\\1");
        textCursor.insertText(selectedText);
    }
    else {
        textCursor.insertText("<" + tag + ">" + selectedText + "</" + tag + ">");
    }
}

void MainWindow::SubTextChanged() {
    isSubApplied = false;
}

void MainWindow::SubCursorPosChanged() {
    QTextCursor textCursor = ui->SubtitleTextEdit->textCursor();

    int TagsLength = 4;
    QString Tags[] = { "b", "i", "u", "s" };
    QToolButton *Buttons[] = { ui->SubBoldButton, ui->SubItalicButton, ui->SubUnderlineButton, ui->SubStrikeoutButton };

    if (!textCursor.hasSelection()) {
        for (int i = 0; i < TagsLength; i++) Buttons[i]->setChecked(false);

        return;
    }

    QString selectedText(textCursor.selectedText());

    for (int i = 0; i < TagsLength; i++) {
        QString regExp("<" + Tags[i] + ">(.*)</" + Tags[i] + ">");
        Buttons[i]->setChecked(QRegExp(regExp).exactMatch(selectedText));
    }
}

void MainWindow::SubBoldClicked() {
    SubTextToggleTag("b");
}

void MainWindow::SubItalicClicked() {
    SubTextToggleTag("i");
}

void MainWindow::SubUnderlineClicked() {
    SubTextToggleTag("u");
}

void MainWindow::SubStrikeoutClicked() {
    SubTextToggleTag("s");
}

void MainWindow::ApplySubtitle() {
    if (!hasFileOpen) {
        QMessageBox::critical(this, "Error", "Open a subtitle file first");
        return;
    }

    QString SubText = ui->SubtitleTextEdit->toPlainText();
    QTime SubShowTime = ui->ShowSubTimeEdit->time();
    QTime SubHideTime = ui->HideSubTimeEdit->time();

    SubtitleItem SubItem(SubShowTime, SubHideTime, SubText);

    if (SubText.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Fill all the required fields");
        return;
    }

    if (EditingSubtitleIndex < 0) {
        subtitlesModel->setItem(Subtitles.size(), 0, new QStandardItem(SubShowTime.toString("hh:mm:ss,zzz")));
        subtitlesModel->setItem(Subtitles.size(), 1, new QStandardItem(SubHideTime.toString("hh:mm:ss,zzz")));
        subtitlesModel->setItem(Subtitles.size(), 2, new QStandardItem(SubText));

        Subtitles.push_back(SubItem);
        UndoItems.append(UndoItem(SubItem, UndoItem::ItemType::ADD));
    }
    else {
        subtitlesModel->setItem(EditingSubtitleIndex, 0, new QStandardItem(SubShowTime.toString("hh:mm:ss,zzz")));
        subtitlesModel->setItem(EditingSubtitleIndex, 1, new QStandardItem(SubHideTime.toString("hh:mm:ss,zzz")));
        subtitlesModel->setItem(EditingSubtitleIndex, 2, new QStandardItem(SubText));

        UndoItems.append(UndoItem(Subtitles.at(EditingSubtitleIndex), SubItem, UndoItem::ItemType::EDIT));
        Subtitles.replace(EditingSubtitleIndex, SubItem);
    }

    subtitlesModel->sort(0);
    std::sort(Subtitles.begin(), Subtitles.end(), SubtitleItem::SortByShowTime);

    isSubApplied = true;
    SetIsSaved(false);

    ShowAvailableSub();

    ui->SubtitleTextEdit->clearFocus();
}

void MainWindow::RemoveSubtitle() {
    if (!hasFileOpen) {
        QMessageBox::critical(this, "Error", "Open a subtitle file first");
        return;
    }

    if (EditingSubtitleIndex < 0) {
        QMessageBox::warning(this, "Warning", "Please, select a subtitle first");
        return;
    }

    subtitlesModel->removeRow(EditingSubtitleIndex);
    subtitlesModel->sort(0);

    UndoItems.append(UndoItem(Subtitles.at(EditingSubtitleIndex), UndoItem::ItemType::REMOVE));

    Subtitles.removeAt(EditingSubtitleIndex);
    std::sort(Subtitles.begin(), Subtitles.end(), SubtitleItem::SortByShowTime);

    ui->SubtitleTextEdit->setPlainText(QString());
    ui->ShowSubTimeEdit->setTime(QTime());
    ui->HideSubTimeEdit->setTime(QTime());

    EditingSubtitleIndex = -1;
    isSubApplied = true;

    SetIsSaved(false);

    ui->SubtitleTextEdit->clearFocus();
}
