; -- SPF Framework Installer Script --
; See https://jrsoftware.org/ishelp/ for documentation

#define MyAppName "SPF Framework"
#define MyAppVersion "1.0.3"
#define MyAppPublisher "TrackAndTruckDevs"
#define MyAppURL "https://github.com/TrackAndTruckDevs/SPF-Framework"
#define MySteamAppIdATS "270880"
#define MySteamAppIdETS2 "227300"

[Setup]
AppId={{11a9f0ff-558b-4844-8804-7e9e2328ba27}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
SetupIconFile=icon.ico
DefaultDirName={autopf}\{#MyAppName}
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputBaseFilename=spf-framework
Compression=lzma
SolidCompression=yes
WizardStyle=modern polar includetitlebar
UninstallDisplayIcon={app}\bin\win_x64\plugins\spf-framework.dll
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "ukrainian"; MessagesFile: "compiler:Languages\Ukrainian.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "dutch"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "turkish"; MessagesFile: "compiler:Languages\Turkish.isl"
Name: "czech"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "hungarian"; MessagesFile: "compiler:Languages\Hungarian.isl"
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"
Name: "norwegian"; MessagesFile: "compiler:Languages\Norwegian.isl"
Name: "danish"; MessagesFile: "compiler:Languages\Danish.isl"
Name: "finnish"; MessagesFile: "compiler:Languages\Finnish.isl"

[Tasks]
Name: "InstallForATS"; Description: "{cm:TaskInstallForATS}"; GroupDescription: "{cm:TaskGroupGames}"; Flags: checkablealone
Name: "InstallForETS2"; Description: "{cm:TaskInstallForETS2}"; GroupDescription: "{cm:TaskGroupGames}"; Flags: checkablealone

Name: "CreatePluginsShortcut"; Description: "{cm:CreatePluginsShortcut}"; Flags: unchecked

[Files]
Source: "dist\spf-framework.dll"; DestDir: "{code:GetInstallDir|ATS}\bin\win_x64\plugins"; Check: IsGameSelected('ATS'); Flags: replacesameversion
Source: "dist\spfAssets\*"; DestDir: "{code:GetInstallDir|ATS}\bin\win_x64\plugins\spfAssets"; Check: IsGameSelected('ATS'); Flags: recursesubdirs createallsubdirs
Source: "dist\spf-framework.dll"; DestDir: "{code:GetInstallDir|ETS2}\bin\win_x64\plugins"; Check: IsGameSelected('ETS2'); Flags: replacesameversion
Source: "dist\spfAssets\*"; DestDir: "{code:GetInstallDir|ETS2}\bin\win_x64\plugins\spfAssets"; Check: IsGameSelected('ETS2'); Flags: recursesubdirs createallsubdirs

[Dirs]
; Create the user plugins directory but never remove it on uninstall
Name: "{code:GetInstallDir|ATS}\bin\win_x64\plugins\spfPlugins"; Check: IsGameSelected('ATS'); Flags: uninsneveruninstall
Name: "{code:GetInstallDir|ETS2}\bin\win_x64\plugins\spfPlugins"; Check: IsGameSelected('ETS2'); Flags: uninsneveruninstall

[Icons]
Name: "{autodesktop}\{cm:ATSPluginsShortcutName}"; Filename: "{code:GetInstallDir|ATS}\bin\win_x64\plugins"; Tasks: CreatePluginsShortcut; Check: IsGameSelected('ATS')
Name: "{autodesktop}\{cm:ETS2PluginsShortcutName}"; Filename: "{code:GetInstallDir|ETS2}\bin\win_x64\plugins"; Tasks: CreatePluginsShortcut; Check: IsGameSelected('ETS2')

[Registry]
; Write version info after successful installation
Root: HKLM; Subkey: "Software\SPF_Framework"; ValueType: string; ValueName: "Version"; ValueData: "{#MyAppVersion}"; Check: IsGameSelected('ATS') or IsGameSelected('ETS2')

[CustomMessages]
english.TaskInstallForATS=Install for the game American Truck Simulator
ukrainian.TaskInstallForATS=Встановити для гри American Truck Simulator
german.TaskInstallForATS=Für das Spiel American Truck Simulator installieren
french.TaskInstallForATS=Installer pour le jeu American Truck Simulator
spanish.TaskInstallForATS=Instalar para el juego American Truck Simulator
polish.TaskInstallForATS=Zainstaluj dla gry American Truck Simulator
russian.TaskInstallForATS=Установить для игры American Truck Simulator
brazilianportuguese.TaskInstallForATS=Instalar para o jogo American Truck Simulator
portuguese.TaskInstallForATS=Instalar para o jogo American Truck Simulator
italian.TaskInstallForATS=Installa per il gioco American Truck Simulator
dutch.TaskInstallForATS=Installeren voor het spel American Truck Simulator
turkish.TaskInstallForATS=American Truck Simulator oyunu için yükle
czech.TaskInstallForATS=Instalovat pro hru American Truck Simulator
hungarian.TaskInstallForATS=Telepítés az American Truck Simulator játékhoz
chinesesimplified.TaskInstallForATS=为游戏《美国卡车模拟》安装
japanese.TaskInstallForATS=ゲーム「American Truck Simulator」用にインストール
korean.TaskInstallForATS=게임 American Truck Simulator용으로 설치
norwegian.TaskInstallForATS=Installer for spillet American Truck Simulator
danish.TaskInstallForATS=Installer for spillet American Truck Simulator
finnish.TaskInstallForATS=Asenna peliin American Truck Simulator
english.TaskInstallForETS2=Install for the game Euro Truck Simulator 2
ukrainian.TaskInstallForETS2=Встановити для гри Euro Truck Simulator 2
german.TaskInstallForETS2=Für das Spiel Euro Truck Simulator 2 installieren
french.TaskInstallForETS2=Installer pour le jeu Euro Truck Simulator 2
spanish.TaskInstallForETS2=Instalar para el juego Euro Truck Simulator 2
polish.TaskInstallForETS2=Zainstaluj dla gry Euro Truck Simulator 2
russian.TaskInstallForETS2=Установить для игры Euro Truck Simulator 2
brazilianportuguese.TaskInstallForETS2=Instalar para o jogo Euro Truck Simulator 2
portuguese.TaskInstallForETS2=Instalar para o jogo Euro Truck Simulator 2
italian.TaskInstallForETS2=Installa per il gioco Euro Truck Simulator 2
dutch.TaskInstallForETS2=Installeren voor het spel Euro Truck Simulator 2
turkish.TaskInstallForETS2=Euro Truck Simulator 2 oyunu için yükle
czech.TaskInstallForETS2=Instalovat pro hru Euro Truck Simulator 2
hungarian.TaskInstallForETS2=Telepítés az Euro Truck Simulator 2 játékhoz
chinesesimplified.TaskInstallForETS2=为游戏《欧洲卡车模拟 2》安装
japanese.TaskInstallForETS2=ゲーム「Euro Truck Simulator 2」用にインストール
korean.TaskInstallForETS2=게임 Euro Truck Simulator 2용으로 설치
norwegian.TaskInstallForETS2=Installer for spillet Euro Truck Simulator 2
danish.TaskInstallForETS2=Installer for spillet Euro Truck Simulator 2
finnish.TaskInstallForETS2=Asenna peliin Euro Truck Simulator 2
english.TaskGroupGames=Games:
ukrainian.TaskGroupGames=Ігри:
german.TaskGroupGames=Spiele:
french.TaskGroupGames=Jeux :
spanish.TaskGroupGames=Juegos:
polish.TaskGroupGames=Gry:
russian.TaskGroupGames=Игры:
brazilianportuguese.TaskGroupGames=Jogos:
portuguese.TaskGroupGames=Jogos:
italian.TaskGroupGames=Giochi:
dutch.TaskGroupGames=Spellen:
turkish.TaskGroupGames=Oyunlar:
czech.TaskGroupGames=Hry:
hungarian.TaskGroupGames=Játékok:
chinesesimplified.TaskGroupGames=游戏：
japanese.TaskGroupGames=ゲーム：
korean.TaskGroupGames=게임:
norwegian.TaskGroupGames=Spill:
danish.TaskGroupGames=Spil:
finnish.TaskGroupGames=Pelit:
english.PathPageTitle=Confirm Installation Paths
ukrainian.PathPageTitle=Підтвердіть шляхи встановлення
german.PathPageTitle=Installationspfade bestätigen
french.PathPageTitle=Confirmer les chemins d'installation
spanish.PathPageTitle=Confirmar rutas de instalación
polish.PathPageTitle=Potwierdź ścieżki instalacji
russian.PathPageTitle=Подтвердите пути установки
brazilianportuguese.PathPageTitle=Confirmar Caminhos de Instalação
portuguese.PathPageTitle=Confirmar Caminhos de Instalação
italian.PathPageTitle=Conferma i percorsi di installazione
dutch.PathPageTitle=Installatiepaden bevestigen
turkish.PathPageTitle=Kurulum Yollarını Onayla
czech.PathPageTitle=Potvrďte instalační cesty
hungarian.PathPageTitle=Telepítési útvonalak megerősítése
chinesesimplified.PathPageTitle=确认安装路径
japanese.PathPageTitle=インストールパスの確認
korean.PathPageTitle=설치 경로 확인
norwegian.PathPageTitle=Bekreft installasjonsstier
danish.PathPageTitle=Bekræft installationsstier
finnish.PathPageTitle=Vahvista asennuspolut
english.PathPageDescription=Please confirm the installation paths for the selected games.
ukrainian.PathPageDescription=Будь ласка, підтвердіть шляхи встановлення для обраних ігор.
german.PathPageDescription=Bitte bestätigen Sie die Installationspfade für die ausgewählten Spiele.
french.PathPageDescription=Veuillez confirmer les chemins d'installation pour les jeux sélectionnés.
spanish.PathPageDescription=Por favor, confirme las rutas de instalación para los juegos seleccionados.
polish.PathPageDescription=Proszę potwierdzić ścieżki instalacji dla wybranych gier.
russian.PathPageDescription=Пожалуйста, подтвердите пути установки для выбранных игр.
brazilianportuguese.PathPageDescription=Por favor, confirme os caminhos de instalação para os jogos selecionados.
portuguese.PathPageDescription=Por favor, confirme os caminhos de instalação para os jogos selecionados.
italian.PathPageDescription=Si prega di confermare i percorsi di installazione per i giochi selezionati.
dutch.PathPageDescription=Bevestig de installatiepaden voor de geselecteerde spellen.
turkish.PathPageDescription=Lütfen seçilen oyunlar için kurulum yollarını onaylayın.
czech.PathPageDescription=Potvrďte prosím instalační cesty pro vybrané hry.
hungarian.PathPageDescription=Kérjük, erősítse meg a kiválasztott játékok telepítési útvonalait.
chinesesimplified.PathPageDescription=请确认为所选游戏选择的安装路径。
japanese.PathPageDescription=選択したゲームのインストールパスを確認してください。
korean.PathPageDescription=선택한 게임의 설치 경로를 확인하십시오.
norwegian.PathPageDescription=Vennligst bekreft installasjonsstiene for de valgte spillene.
danish.PathPageDescription=Bekræft venligst installationsstierne for de valgte spil.
finnish.PathPageDescription=Vahvista valittujen pelien asennuspolut.
english.BrowseButtonCaption=Browse...
ukrainian.BrowseButtonCaption=Огляд...
german.BrowseButtonCaption=Durchsuchen...
french.BrowseButtonCaption=Parcourir...
spanish.BrowseButtonCaption=Examinar...
polish.BrowseButtonCaption=Przeglądaj...
russian.BrowseButtonCaption=Обзор...
brazilianportuguese.BrowseButtonCaption=Procurar...
portuguese.BrowseButtonCaption=Procurar...
italian.BrowseButtonCaption=Sfoglia...
dutch.BrowseButtonCaption=Bladeren...
turkish.BrowseButtonCaption=Gözat...
czech.BrowseButtonCaption=Procházet...
hungarian.BrowseButtonCaption=Tallózás...
chinesesimplified.BrowseButtonCaption=浏览...
japanese.BrowseButtonCaption=参照...
korean.BrowseButtonCaption=찾아보기...
norwegian.BrowseButtonCaption=Bla gjennom...
danish.BrowseButtonCaption=Gennemse...
finnish.BrowseButtonCaption=Selaa...
english.ATSPathLabelCaption=American Truck Simulator Path:
ukrainian.ATSPathLabelCaption=Шлях до American Truck Simulator:
german.ATSPathLabelCaption=Pfad für American Truck Simulator:
french.ATSPathLabelCaption=Chemin d'accès d'American Truck Simulator :
spanish.ATSPathLabelCaption=Ruta de American Truck Simulator:
polish.ATSPathLabelCaption=Ścieżka do American Truck Simulator:
russian.ATSPathLabelCaption=Путь к American Truck Simulator:
brazilianportuguese.ATSPathLabelCaption=Caminho do American Truck Simulator:
portuguese.ATSPathLabelCaption=Caminho do American Truck Simulator:
italian.ATSPathLabelCaption=Percorso di American Truck Simulator:
dutch.ATSPathLabelCaption=Pad van American Truck Simulator:
turkish.ATSPathLabelCaption=American Truck Simulator Yolu:
czech.ATSPathLabelCaption=Cesta k American Truck Simulator:
hungarian.ATSPathLabelCaption=American Truck Simulator útvonala:
chinesesimplified.ATSPathLabelCaption=《美国卡车模拟》路径：
japanese.ATSPathLabelCaption=American Truck Simulator のパス:
korean.ATSPathLabelCaption=American Truck Simulator 경로:
norwegian.ATSPathLabelCaption=Sti for American Truck Simulator:
danish.ATSPathLabelCaption=Sti til American Truck Simulator:
finnish.ATSPathLabelCaption=American Truck Simulatorin polku:
english.ETS2PathLabelCaption=Euro Truck Simulator 2 Path:
ukrainian.ETS2PathLabelCaption=Шлях до Euro Truck Simulator 2:
german.ETS2PathLabelCaption=Pfad für Euro Truck Simulator 2:
french.ETS2PathLabelCaption=Chemin d'accès d'Euro Truck Simulator 2 :
spanish.ETS2PathLabelCaption=Ruta de Euro Truck Simulator 2:
polish.ETS2PathLabelCaption=Ścieżka do Euro Truck Simulator 2:
russian.ETS2PathLabelCaption=Путь к Euro Truck Simulator 2:
brazilianportuguese.ETS2PathLabelCaption=Caminho do Euro Truck Simulator 2:
portuguese.ETS2PathLabelCaption=Caminho do Euro Truck Simulator 2:
italian.ETS2PathLabelCaption=Percorso di Euro Truck Simulator 2:
dutch.ETS2PathLabelCaption=Pad van Euro Truck Simulator 2:
turkish.ETS2PathLabelCaption=Euro Truck Simulator 2 Yolu:
czech.ETS2PathLabelCaption=Cesta k Euro Truck Simulator 2:
hungarian.ETS2PathLabelCaption=Euro Truck Simulator 2 útvonala:
chinesesimplified.ETS2PathLabelCaption=《欧洲卡车模拟 2》路径：
japanese.ETS2PathLabelCaption=Euro Truck Simulator 2 のパス:
korean.ETS2PathLabelCaption=Euro Truck Simulator 2 경로:
norwegian.ETS2PathLabelCaption=Sti for Euro Truck Simulator 2:
danish.ETS2PathLabelCaption=Sti til Euro Truck Simulator 2:
finnish.ETS2PathLabelCaption=Euro Truck Simulator 2:n polku:
english.SelectATSFolderCaption=Select American Truck Simulator Folder
ukrainian.SelectATSFolderCaption=Виберіть папку American Truck Simulator
german.SelectATSFolderCaption=Wählen Sie den Ordner für American Truck Simulator
french.SelectATSFolderCaption=Sélectionnez le dossier d'American Truck Simulator
spanish.SelectATSFolderCaption=Seleccione la carpeta de American Truck Simulator
polish.SelectATSFolderCaption=Wybierz folder American Truck Simulator
russian.SelectATSFolderCaption=Выберите папку American Truck Simulator
brazilianportuguese.SelectATSFolderCaption=Selecione a Pasta do American Truck Simulator
portuguese.SelectATSFolderCaption=Selecione a Pasta do American Truck Simulator
italian.SelectATSFolderCaption=Seleziona la cartella di American Truck Simulator
dutch.SelectATSFolderCaption=Selecteer de map van American Truck Simulator
turkish.SelectATSFolderCaption=American Truck Simulator Klasörünü Seçin
czech.SelectATSFolderCaption=Vyberte složku American Truck Simulator
hungarian.SelectATSFolderCaption=Válassza ki az American Truck Simulator mappáját
chinesesimplified.SelectATSFolderCaption=选择《美国卡车模拟》文件夹
japanese.SelectATSFolderCaption=American Truck Simulator のフォルダーを選択
korean.SelectATSFolderCaption=American Truck Simulator 폴더 선택
norwegian.SelectATSFolderCaption=Velg American Truck Simulator-mappen
danish.SelectATSFolderCaption=Vælg American Truck Simulator-mappe
finnish.SelectATSFolderCaption=Valitse American Truck Simulator -kansio
english.SelectETS2FolderCaption=Select Euro Truck Simulator 2 Folder
ukrainian.SelectETS2FolderCaption=Виберіть папку Euro Truck Simulator 2
german.SelectETS2FolderCaption=Wählen Sie den Ordner für Euro Truck Simulator 2
french.SelectETS2FolderCaption=Sélectionnez le dossier d'Euro Truck Simulator 2
spanish.SelectETS2FolderCaption=Seleccione la carpeta de Euro Truck Simulator 2
polish.SelectETS2FolderCaption=Wybierz folder Euro Truck Simulator 2
russian.SelectETS2FolderCaption=Выберите папку Euro Truck Simulator 2
brazilianportuguese.SelectETS2FolderCaption=Selecione a Pasta do Euro Truck Simulator 2
portuguese.SelectETS2FolderCaption=Selecione a Pasta do Euro Truck Simulator 2
italian.SelectETS2FolderCaption=Seleziona la cartella di Euro Truck Simulator 2
dutch.SelectETS2FolderCaption=Selecteer de map van Euro Truck Simulator 2
turkish.SelectETS2FolderCaption=Euro Truck Simulator 2 Klasörünü Seçin
czech.SelectETS2FolderCaption=Vyberte složku Euro Truck Simulator 2
hungarian.SelectETS2FolderCaption=Válassza ki az Euro Truck Simulator 2 mappáját
chinesesimplified.SelectETS2FolderCaption=选择《欧洲卡车模拟 2》文件夹
japanese.SelectETS2FolderCaption=Euro Truck Simulator 2 のフォルダーを選択
korean.SelectETS2FolderCaption=Euro Truck Simulator 2 폴더 선택
norwegian.SelectETS2FolderCaption=Velg Euro Truck Simulator 2-mappen
danish.SelectETS2FolderCaption=Vælg Euro Truck Simulator 2-mappe
finnish.SelectETS2FolderCaption=Valitse Euro Truck Simulator 2 -kansio
english.FoundInstalledVersionLog=Found installed version: %s
ukrainian.FoundInstalledVersionLog=Знайдено встановлену версію: %s
german.FoundInstalledVersionLog=Installierte Version gefunden: %s
french.FoundInstalledVersionLog=Version installée trouvée : %s
spanish.FoundInstalledVersionLog=Versión instalada encontrada: %s
polish.FoundInstalledVersionLog=Znaleziono zainstalowaną wersję: %s
russian.FoundInstalledVersionLog=Найдена установленная версия: %s
brazilianportuguese.FoundInstalledVersionLog=Versão instalada encontrada: %s
portuguese.FoundInstalledVersionLog=Versão instalada encontrada: %s
italian.FoundInstalledVersionLog=Versione installata trovata: %s
dutch.FoundInstalledVersionLog=Geïnstalleerde versie gevonden: %s
turkish.FoundInstalledVersionLog=Yüklü sürüm bulundu: %s
czech.FoundInstalledVersionLog=Nalezena nainstalovaná verze: %s
hungarian.FoundInstalledVersionLog=Telepített verzió: %s
chinesesimplified.FoundInstalledVersionLog=找到已安装的版本：%s
japanese.FoundInstalledVersionLog=インストール済みのバージョンが見つかりました: %s
korean.FoundInstalledVersionLog=설치된 버전 찾음: %s
norwegian.FoundInstalledVersionLog=Fant installert versjon: %s
danish.FoundInstalledVersionLog=Installeret version fundet: %s
finnish.FoundInstalledVersionLog=Löytyi asennettu versio: %s
english.ThisInstallerVersionLog=This installer version: %s
ukrainian.ThisInstallerVersionLog=Версія цього інсталятора: %s
german.ThisInstallerVersionLog=Diese Installer-Version: %s
french.ThisInstallerVersionLog=Version de cet installateur : %s
spanish.ThisInstallerVersionLog=Versión de este instalador: %s
polish.ThisInstallerVersionLog=Wersja tego instalatora: %s
russian.ThisInstallerVersionLog=Версия этого установщика: %s
brazilianportuguese.ThisInstallerVersionLog=Versão deste instalador: %s
portuguese.ThisInstallerVersionLog=Versão deste instalador: %s
italian.ThisInstallerVersionLog=Versione di questo programma di installazione: %s
dutch.ThisInstallerVersionLog=Deze installer-versie: %s
turkish.ThisInstallerVersionLog=Bu yükleyici sürümü: %s
czech.ThisInstallerVersionLog=Verze tohoto instalačního programu: %s
hungarian.ThisInstallerVersionLog=Telepítő verziója: %s
chinesesimplified.ThisInstallerVersionLog=此安装程序版本：%s
japanese.ThisInstallerVersionLog=このインストーラーのバージョン: %s
korean.ThisInstallerVersionLog=이 설치 프로그램 버전: %s
norwegian.ThisInstallerVersionLog=Denne installasjonsversjonen: %s
danish.ThisInstallerVersionLog=Denne installationsversions: %s
finnish.ThisInstallerVersionLog=Tämän asennusohjelman versio: %s
english.DowngradeWarning=A newer version (%s) of the SPF Framework is already installed. You are trying to install an older version (%s). Downgrading is not recommended and may cause issues. Do you want to continue anyway?
ukrainian.DowngradeWarning=Вже встановлено новішу версію (%s) SPF Framework. Ви намагаєтеся встановити старішу версію (%s). Пониження версії не рекомендується і може призвести до проблем. Продовжити?
german.DowngradeWarning=Eine neuere Version (%s) des SPF Framework ist bereits installiert. Sie versuchen, eine ältere Version (%s) zu installieren. Ein Downgrade wird nicht empfohlen und kann zu Problemen führen. Trotzdem fortfahren?
french.DowngradeWarning=Une version plus récente (%s) du Framework SPF est déjà installée. Vous essayez d'installer une version plus ancienne (%s). La rétrogradation n'est pas recommandée et peut causer des problèmes. Voulez-vous continuer quand même ?
spanish.DowngradeWarning=Ya está instalada una versión más reciente (%s) del SPF Framework. Está intentando instalar una versión anterior (%s). No se recomienda la degradación y puede causar problemas. ¿Desea continuar de todos modos?
polish.DowngradeWarning=Nowsza wersja (%s) platformy SPF Framework jest już zainstalowana. Próbujesz zainstalować starszą wersję (%s). Obniżenie wersji nie jest zalecane i może powodować problemy. Czy chcesz kontynuować mimo to?
russian.DowngradeWarning=Уже установлена более новая версия (%s) SPF Framework. Вы пытаетесь установить более старую версию (%s). Понижение версии не рекомендуется и может вызвать проблемы. Все равно продолжить?
brazilianportuguese.DowngradeWarning=Uma versão mais recente (%s) do SPF Framework já está instalada. Você está tentando instalar uma versão mais antiga (%s). O downgrade não é recomendado e pode causar problemas. Deseja continuar mesmo assim?
portuguese.DowngradeWarning=Já está instalada uma versão mais recente (%s) do SPF Framework. Está a tentar instalar uma versão mais antiga (%s). A retrogradação não é recomendada e pode causar problemas. Deseja continuar na mesma?
italian.DowngradeWarning=È già installata una versione più recente (%s) di SPF Framework. Si sta tentando di installare una versione precedente (%s). Il downgrade non è raccomandato e potrebbe causare problemi. Continuare comunque?
dutch.DowngradeWarning=Een nieuwere versie (%s) van het SPF Framework is al geïnstalleerd. U probeert een oudere versie (%s) te installeren. Downgraden wordt niet aanbevolen en kan problemen veroorzaken. Wilt u toch doorgaan?
turkish.DowngradeWarning=SPF Framework'ün daha yeni bir sürümü (%s) zaten yüklü. Daha eski bir sürümü (%s) yüklemeye çalışıyorsunuz. Sürüm düşürme önerilmez ve sorunlara neden olabilir. Yine de devam etmek istiyor musunuz?
czech.DowngradeWarning=Novější verze (%s) SPF Framework je již nainstalována. Pokoušíte se nainstalovat starší verzi (%s). Downgrade se nedoporučuje a může způsobit problémy. Chcete přesto pokračovat?
hungarian.DowngradeWarning=Az SPF Framework egy újabb verziója (%s) már telepítve van. Egy régebbi verziót (%s) próbál telepíteni. A visszalépés nem ajánlott, és problémákat okozhat. Mégis folytatja?
chinesesimplified.DowngradeWarning=已安装较新版本的SPF框架（%s）。您正在尝试安装较旧版本（%s）。不建议降级，并可能导致问题。是否仍要继续？
japanese.DowngradeWarning=新しいバージョンのSPFフレームワーク（%s）が既にインストールされています。古いバージョン（%s）をインストールしようとしています。ダунгレードは推奨されず、問題を引き起こす可能性があります。続行しますか？
korean.DowngradeWarning=SPF 프레임워크의 최신 버전(%s)이 이미 설치되어 있습니다. 이전 버전(%s)을 설치하려고 합니다. 다운그레이드는 권장되지 않으며 문제를 일으킬 수 있습니다. 계속하시겠습니까?
norwegian.DowngradeWarning=En nyere versjon (%s) av SPF Framework er allerede installert. Du prøver å installere en eldre versjon (%s). Nedgradering anbefales ikke og kan føre til problemer. Vil du fortsette likevel?
danish.DowngradeWarning=En nyere version (%s) af SPF Framework er allerede installeret. Du forsøger at installere en ældre version (%s). Nedgradering anbefales ikke og kan forårsage problemer. Vil du fortsætte alligevel?
finnish.DowngradeWarning=Uudempi versio (%s) SPF Frameworkista on jo asennettu. Yrität asentaa vanhempaa versiota (%s). Version alentamista ei suositella, ja se voi aiheuttaa ongelmia. Haluatko jatkaa silti?
english.InvalidATSPathMsg=The selected directory for American Truck Simulator is not valid. Please select the root folder of the game (e.g., "...Steam\steamapps\common\American Truck Simulator").
ukrainian.InvalidATSPathMsg=Обрана директорія для American Truck Simulator недійсна. Будь ласка, виберіть кореневу папку гри (наприклад, "...Steam\steamapps\common\American Truck Simulator").
german.InvalidATSPathMsg=Das ausgewählte Verzeichnis für American Truck Simulator ist ungültig. Bitte wählen Sie den Stammordner des Spiels (z.B. "...Steam\steamapps\common\American Truck Simulator").
french.InvalidATSPathMsg=Le répertoire sélectionné pour American Truck Simulator n'est pas valide. Veuillez sélectionner le dossier racine du jeu (par ex., "...Steam\steamapps\common\American Truck Simulator").
spanish.InvalidATSPathMsg=El directorio seleccionado para American Truck Simulator no es válido. Por favor, seleccione la carpeta raíz del juego (p. ej., "...Steam\steamapps\common\American Truck Simulator").
polish.InvalidATSPathMsg=Wybrany katalog dla American Truck Simulator jest nieprawidłowy. Proszę wybrać główny folder gry (np. "...Steam\steamapps\common\American Truck Simulator").
russian.InvalidATSPathMsg=Выбранный каталог для American Truck Simulator недействителен. Пожалуйста, выберите корневую папку игры (например, "...Steam\steamapps\common\American Truck Simulator").
brazilianportuguese.InvalidATSPathMsg=O diretório selecionado para o American Truck Simulator não é válido. Por favor, selecione a pasta raiz do jogo (ex., "...Steam\steamapps\common\American Truck Simulator").
portuguese.InvalidATSPathMsg=O diretório selecionado para o American Truck Simulator não é válido. Por favor, selecione a pasta raiz do jogo (ex., "...Steam\steamapps\common\American Truck Simulator").
italian.InvalidATSPathMsg=La directory selezionata per American Truck Simulator non è valida. Selezionare la cartella principale del gioco (ad es. "...Steam\steamapps\common\American Truck Simulator").
dutch.InvalidATSPathMsg=De geselecteerde map voor American Truck Simulator is ongeldig. Selecteer de hoofdmap van het spel (bijv. "...Steam\steamapps\common\American Truck Simulator").
turkish.InvalidATSPathMsg=American Truck Simulator için seçilen dizin geçerli değil. Lütfen oyunun kök klasörünü seçin (örneğin, "...Steam\steamapps\common\American Truck Simulator").
czech.InvalidATSPathMsg=Vybraný adresář pro American Truck Simulator není platný. Vyberte prosím kořenový adresář hry (např. "...Steam\steamapps\common\American Truck Simulator").
hungarian.InvalidATSPathMsg=Az American Truck Simulatorhoz kiválasztott könyvtár érvénytelen. Kérjük, válassza ki a játék gyökérkönyvtárát (pl. "...Steam\steamapps\common\American Truck Simulator").
chinesesimplified.InvalidATSPathMsg=所选的《美国卡车模拟》目录无效。请选择游戏的根文件夹（例如：“...Steam\steamapps\common\American Truck Simulator”）。
japanese.InvalidATSPathMsg=American Truck Simulator に選択されたディレクトリは無効です。ゲームのルートフォルダを選択してください (例: 「...Steam\steamapps\common\American Truck Simulator」)。
korean.InvalidATSPathMsg=American Truck Simulator에 대해 선택한 디렉터리가 잘못되었습니다. 게임의 루트 폴더를 선택하십시오(예: "...Steam\steamapps\common\American Truck Simulator").
norwegian.InvalidATSPathMsg=Den valgte mappen for American Truck Simulator er ugyldig. Vennligst velg rotmappen til spillet (f.eks. "...Steam\steamapps\common\American Truck Simulator").
danish.InvalidATSPathMsg=Den valgte mappe for American Truck Simulator er ugyldig. Vælg venligst spillets rodmappe (f.eks. "...Steam\steamapps\common\American Truck Simulator").
finnish.InvalidATSPathMsg=Valittu hakemisto American Truck Simulatorille on virheellinen. Valitse pelin juurikansio (esim. "...Steam\steamapps\common\American Truck Simulator").
english.InvalidETS2PathMsg=The selected directory for Euro Truck Simulator 2 is not valid. Please select the root folder of the game (e.g., "...Steam\steamapps\common\Euro Truck Simulator 2").
ukrainian.InvalidETS2PathMsg=Обрана директорія для Euro Truck Simulator 2 недійсна. Будь ласка, виберіть кореневу папку гри (наприклад, "...Steam\steamapps\common\Euro Truck Simulator 2").
german.InvalidETS2PathMsg=Das ausgewählte Verzeichnis für Euro Truck Simulator 2 ist ungültig. Bitte wählen Sie den Stammordner des Spiels (z.B. "...Steam\steamapps\common\Euro Truck Simulator 2").
french.InvalidETS2PathMsg=Le répertoire sélectionné pour Euro Truck Simulator 2 n'est pas valide. Veuillez sélectionner le dossier racine du jeu (par ex., "...Steam\steamapps\common\Euro Truck Simulator 2").
spanish.InvalidETS2PathMsg=El directorio seleccionado para Euro Truck Simulator 2 no es válido. Por favor, seleccione la carpeta raíz del juego (p. ej., "...Steam\steamapps\common\Euro Truck Simulator 2").
polish.InvalidETS2PathMsg=Wybrany katalog dla Euro Truck Simulator 2 jest nieprawidłowy. Proszę wybrać główny folder gry (np. "...Steam\steamapps\common\Euro Truck Simulator 2").
russian.InvalidETS2PathMsg=Выбранный каталог для Euro Truck Simulator 2 недействителен. Пожалуйста, выберите корневую папку игры (например, "...Steam\steamapps\common\Euro Truck Simulator 2").
brazilianportuguese.InvalidETS2PathMsg=O diretório selecionado para o Euro Truck Simulator 2 não é válido. Por favor, selecione a pasta raiz do jogo (ex., "...Steam\steamapps\common\Euro Truck Simulator 2").
portuguese.InvalidETS2PathMsg=O diretório selecionado para o Euro Truck Simulator 2 não é válido. Por favor, selecione a pasta raiz do jogo (ex., "...Steam\steamapps\common\Euro Truck Simulator 2").
italian.InvalidETS2PathMsg=La directory selezionata per Euro Truck Simulator 2 non è valida. Selezionare la cartella principale del gioco (ad es. "...Steam\steamapps\common\Euro Truck Simulator 2").
dutch.InvalidETS2PathMsg=De geselecteerde map voor Euro Truck Simulator 2 is ongeldig. Selecteer de hoofdmap van het spel (bijv. "...Steam\steamapps\common\Euro Truck Simulator 2").
turkish.InvalidETS2PathMsg=Euro Truck Simulator 2 için seçilen dizin geçerli değil. Lütfen oyunun kök klasörünü seçin (örneğin, "...Steam\steamapps\common\Euro Truck Simulator 2").
czech.InvalidETS2PathMsg=Vybraný adresář pro Euro Truck Simulator 2 není platný. Vyberte prosím kořenový adresář hry (např. "...Steam\steamapps\common\Euro Truck Simulator 2").
hungarian.InvalidETS2PathMsg=Az Euro Truck Simulator 2-hez kiválasztott könyvtár érvénytelen. Kérjük, válassza ki a játék gyökérkönyvtárát (pl. "...Steam\steamapps\common\Euro Truck Simulator 2").
chinesesimplified.InvalidETS2PathMsg=所选的《欧洲卡车模拟 2》目录无效。请选择游戏的根文件夹（例如：“...Steam\steamapps\common\Euro Truck Simulator 2”）。
japanese.InvalidETS2PathMsg=Euro Truck Simulator 2 に選択されたディレクトリは無効です。ゲームのルートフォルダを選択してください (例: 「...Steam\steamapps\common\Euro Truck Simulator 2」)。
korean.InvalidETS2PathMsg=Euro Truck Simulator 2에 대해 선택한 디렉터리가 잘못되었습니다. 게임의 루트 폴더를 선택하십시오(예: "...Steam\steamapps\common\Euro Truck Simulator 2").
norwegian.InvalidETS2PathMsg=Den valgte mappen for Euro Truck Simulator 2 er ugyldig. Vennligst velg rotmappen til spillet (f.eks. "...Steam\steamapps\common\Euro Truck Simulator 2").
danish.InvalidETS2PathMsg=Den valgte mappe for Euro Truck Simulator 2 er ugyldig. Vælg venligst spillets rodmappe (f.eks. "...Steam\steamapps\common\Euro Truck Simulator 2").
finnish.InvalidETS2PathMsg=Valittu hakemisto Euro Truck Simulator 2:lle on virheellinen. Valitse pelin juurikansio (esim. "...Steam\steamapps\common\Euro Truck Simulator 2").
english.InstallLocationsIntro=The installer will install the framework to the following locations:
ukrainian.InstallLocationsIntro=Інсталятор встановить фреймворк у наступні розташування:
german.InstallLocationsIntro=Das Installationsprogramm installiert das Framework an den folgenden Orten:
french.InstallLocationsIntro=Le programme d'installation installera le framework aux emplacements suivants :
spanish.InstallLocationsIntro=El instalador instalará el framework en las siguientes ubicaciones:
polish.InstallLocationsIntro=Instalator zainstaluje framework w następujących lokalizacjach:
russian.InstallLocationsIntro=Установщик установит фреймворк в следующие места:
brazilianportuguese.InstallLocationsIntro=O instalador irá instalar o framework nos seguintes locais:
portuguese.InstallLocationsIntro=O instalador irá instalar o framework nos seguintes locais:
italian.InstallLocationsIntro=Il programma di installazione installerà il framework nelle seguenti posizioni:
dutch.InstallLocationsIntro=Het installatieprogramma installeert het framework op de volgende locaties:
turkish.InstallLocationsIntro=Yükleyici, çerçeveyi aşağıdaki konumlara kuracaktır:
czech.InstallLocationsIntro=Instalační program nainstaluje framework na následující místa:
hungarian.InstallLocationsIntro=A telepítő a következő helyekre telepíti a keretrendszert:
chinesesimplified.InstallLocationsIntro=安装程序将把框架安装到以下位置：
japanese.InstallLocationsIntro=インストーラーは、次の場所にフレームワークをインストールします:
korean.InstallLocationsIntro=설치 프로그램이 다음 위치에 프레임워크를 설치합니다:
norwegian.InstallLocationsIntro=Installasjonsprogrammet vil installere rammeverket på følgende steder:
danish.InstallLocationsIntro=Installationsprogrammet vil installere frameworket på følgende steder:
finnish.InstallLocationsIntro=Asennusohjelma asentaa kehyksen seuraaviin sijainteihin:
english.NoGameSelectedMsg=No game selected. No changes will be made.
ukrainian.NoGameSelectedMsg=Жодна гра не обрана. Зміни не будуть внесені.
german.NoGameSelectedMsg=Kein Spiel ausgewählt. Es werden keine Änderungen vorgenommen.
french.NoGameSelectedMsg=Aucun jeu sélectionné. Aucune modification ne sera apportée.
spanish.NoGameSelectedMsg=No se ha seleccionado ningún juego. No se realizarán cambios.
polish.NoGameSelectedMsg=Nie wybrano żadnej gry. Żadne zmiany nie zostaną wprowadzone.
russian.NoGameSelectedMsg=Игра не выбрана. Никаких изменений не будет.
brazilianportuguese.NoGameSelectedMsg=Nenhum jogo selecionado. Nenhuma alteração será feita.
portuguese.NoGameSelectedMsg=Nenhum jogo selecionado. Nenhuma alteração será feita.
italian.NoGameSelectedMsg=Nessun gioco selezionato. Non verranno apportate modifiche.
dutch.NoGameSelectedMsg=Geen spel geselecteerd. Er worden geen wijzigingen aangebracht.
turkish.NoGameSelectedMsg=Oyun seçilmedi. Hiçbir değişiklik yapılmayacak.
czech.NoGameSelectedMsg=Není vybrána žádná hra. Nebudou provedeny žádné změny.
hungarian.NoGameSelectedMsg=Nincs játék kiválasztva. Nem történnek módosítások.
chinesesimplified.NoGameSelectedMsg=未选择任何游戏。不会进行任何更改。
japanese.NoGameSelectedMsg=ゲームが選択されていません。変更は加えられません。
korean.NoGameSelectedMsg=선택된 게임이 없습니다. 변경 사항이 없습니다.
norwegian.NoGameSelectedMsg=Ingen spill valgt. Ingen endringer vil bli gjort.
danish.NoGameSelectedMsg=Intet spil valgt. Der vil ikke blive foretaget nogen ændringer.
finnish.NoGameSelectedMsg=Peliä ei ole valittu. Muutoksia ei tehdä.
english.CreatePluginsShortcut=Create desktop shortcut for plugin folders
ukrainian.CreatePluginsShortcut=Створити ярлик на робочому столі для папок з плагінами
german.CreatePluginsShortcut=Desktop-Verknüpfung für Plugin-Ordner erstellen
french.CreatePluginsShortcut=Créer un raccourci sur le bureau pour les dossiers de plugins
spanish.CreatePluginsShortcut=Crear acceso directo en el escritorio para las carpetas de plugins
polish.CreatePluginsShortcut=Utwórz skrót na pulpicie do folderów z wtyczkami
russian.CreatePluginsShortcut=Создать ярлык на рабочем столе для папок с плагинами
brazilianportuguese.CreatePluginsShortcut=Criar atalho na área de trabalho para pastas de plugins
portuguese.CreatePluginsShortcut=Criar atalho no ambiente de trabalho para as pastas de plugins
italian.CreatePluginsShortcut=Crea un collegamento sul desktop per le cartelle dei plugin
dutch.CreatePluginsShortcut=Snelkoppeling op het bureaublad maken voor plugin-mappen
turkish.CreatePluginsShortcut=Eklenti klasörleri için masaüstü kısayolu oluştur
czech.CreatePluginsShortcut=Vytvořit zástupce na ploše pro složky s pluginy
hungarian.CreatePluginsShortcut=Asztali parancsikon létrehozása a bővítménymappákhoz
chinesesimplified.CreatePluginsShortcut=为插件文件夹创建桌面快捷方式
japanese.CreatePluginsShortcut=プラグインフォルダ用のデスクトップショートカットを作成
korean.CreatePluginsShortcut=플러그인 폴더용 바탕 화면 바로 가기 만들기
norwegian.CreatePluginsShortcut=Opprett skrivebordssnarvei for plugin-mapper
danish.CreatePluginsShortcut=Opret skrivebordsgenvej til plugin-mapper
finnish.CreatePluginsShortcut=Luo työpöydän pikakuvake lisäosakansioille
english.ATSPluginsShortcutName=ATS Plugin Folder
ukrainian.ATSPluginsShortcutName=Папка з плагінами ATS
german.ATSPluginsShortcutName=ATS-Plugin-Ordner
french.ATSPluginsShortcutName=Dossier des plugins ATS
spanish.ATSPluginsShortcutName=Carpeta de plugins de ATS
polish.ATSPluginsShortcutName=Folder z wtyczkami ATS
russian.ATSPluginsShortcutName=Папка с плагинами ATS
brazilianportuguese.ATSPluginsShortcutName=Pasta de Plugins do ATS
portuguese.ATSPluginsShortcutName=Pasta de Plugins do ATS
italian.ATSPluginsShortcutName=Cartella plugin ATS
dutch.ATSPluginsShortcutName=ATS Plugin-map
turkish.ATSPluginsShortcutName=ATS Eklenti Klasörü
czech.ATSPluginsShortcutName=Složka s pluginy ATS
hungarian.ATSPluginsShortcutName=ATS bővítménymappa
chinesesimplified.ATSPluginsShortcutName=ATS 插件文件夹
japanese.ATSPluginsShortcutName=ATS プラグインフォルダ
korean.ATSPluginsShortcutName=ATS 플러그인 폴더
norwegian.ATSPluginsShortcutName=ATS Plugin-mappe
danish.ATSPluginsShortcutName=ATS Plugin-mappe
finnish.ATSPluginsShortcutName=ATS-lisäosakansio
english.ETS2PluginsShortcutName=ETS2 Plugin Folder
ukrainian.ETS2PluginsShortcutName=Папка з плагінами ETS2
german.ETS2PluginsShortcutName=ETS2-Plugin-Ordner
french.ETS2PluginsShortcutName=Dossier des plugins ETS2
spanish.ETS2PluginsShortcutName=Carpeta de plugins de ETS2
polish.ETS2PluginsShortcutName=Folder z wtyczkami ETS2
russian.ETS2PluginsShortcutName=Папка с плагинами ETS2
brazilianportuguese.ETS2PluginsShortcutName=Pasta de Plugins do ETS2
portuguese.ETS2PluginsShortcutName=Pasta de Plugins do ETS2
italian.ETS2PluginsShortcutName=Cartella plugin ETS2
dutch.ETS2PluginsShortcutName=ETS2 Plugin-map
turkish.ETS2PluginsShortcutName=ETS2 Eklenti Klasörü
czech.ETS2PluginsShortcutName=Složka s pluginy ETS2
hungarian.ETS2PluginsShortcutName=ETS2 bővítménymappa
chinesesimplified.ETS2PluginsShortcutName=ETS2 插件文件夹
japanese.ETS2PluginsShortcutName=ETS2 プラグインフォルダ
korean.ETS2PluginsShortcutName=ETS2 플러그인 폴더
norwegian.ETS2PluginsShortcutName=ETS2 Plugin-mappe
danish.ETS2PluginsShortcutName=ETS2 Plugin-mappe
finnish.ETS2PluginsShortcutName=ETS2-lisäosakansio

[Code]
var
  ATSPath, ETS2Path: String;
  PathsPage: TWizardPage;
  ATSPathEdit, ETS2PathEdit: TEdit;
  ATSBrowseButton, ETS2BrowseButton: TButton;
  PathLabel: TLabel;

procedure OnBrowseATS(Sender: TObject);
var
  Path: String;
begin
  Path := ATSPathEdit.Text;
  if BrowseForFolder(CustomMessage('SelectATSFolderCaption'), Path, False) then
    ATSPathEdit.Text := Path;
end;

procedure OnBrowseETS2(Sender: TObject);
var
  Path: String;
begin
  Path := ETS2PathEdit.Text;
  if BrowseForFolder(CustomMessage('SelectETS2FolderCaption'), Path, False) then
    ETS2PathEdit.Text := Path;
end;

procedure CreatePathEditorPage;
begin
  PathsPage := CreateCustomPage(wpSelectTasks, CustomMessage('PathPageTitle'), CustomMessage('PathPageDescription'));

  if WizardIsTaskSelected('InstallForATS') then
  begin
    PathLabel := TLabel.Create(WizardForm);
    PathLabel.Caption := CustomMessage('ATSPathLabelCaption');
    PathLabel.Parent := PathsPage.Surface;
    PathLabel.Top := 10;
    PathLabel.Left := 10;

    ATSPathEdit := TEdit.Create(WizardForm);
    ATSPathEdit.Parent := PathsPage.Surface;
    ATSPathEdit.Top := PathLabel.Top + PathLabel.Height + 5;
    ATSPathEdit.Left := 10;
    ATSPathEdit.Width := PathsPage.Surface.Width - 90;
    ATSPathEdit.Text := ATSPath;

    ATSBrowseButton := TButton.Create(WizardForm);
    ATSBrowseButton.Parent := PathsPage.Surface;
    ATSBrowseButton.Top := ATSPathEdit.Top;
    ATSBrowseButton.Left := ATSPathEdit.Left + ATSPathEdit.Width + 10;
    ATSBrowseButton.Width := 75;
    ATSBrowseButton.Caption := CustomMessage('BrowseButtonCaption');
    ATSBrowseButton.OnClick := @OnBrowseATS;
  end;

  if WizardIsTaskSelected('InstallForETS2') then
  begin
    PathLabel := TLabel.Create(WizardForm);
    PathLabel.Caption := CustomMessage('ETS2PathLabelCaption');
    PathLabel.Parent := PathsPage.Surface;
    if WizardIsTaskSelected('InstallForATS') then
      PathLabel.Top := ATSPathEdit.Top + ATSPathEdit.Height + 10
    else
      PathLabel.Top := 10;
    PathLabel.Left := 10;

    ETS2PathEdit := TEdit.Create(WizardForm);
    ETS2PathEdit.Parent := PathsPage.Surface;
    ETS2PathEdit.Top := PathLabel.Top + PathLabel.Height + 5;
    ETS2PathEdit.Left := 10;
    ETS2PathEdit.Width := PathsPage.Surface.Width - 90;
    ETS2PathEdit.Text := ETS2Path;

    ETS2BrowseButton := TButton.Create(WizardForm);
    ETS2BrowseButton.Parent := PathsPage.Surface;
    ETS2BrowseButton.Top := ETS2PathEdit.Top;
    ETS2BrowseButton.Left := ETS2PathEdit.Left + ETS2PathEdit.Width + 10;
    ETS2BrowseButton.Width := 75;
    ETS2BrowseButton.Caption := CustomMessage('BrowseButtonCaption');
    ETS2BrowseButton.OnClick := @OnBrowseETS2;
  end;
end;

// Function to compare version strings (e.g., "1.2.0" vs "1.10.0")
function CompareVersion(V1, V2: String): Integer;
var
  I, N1, N2: Integer;
  S1, S2: String;
begin
  Result := 0;
  while (Result = 0) and ((V1 <> '') or (V2 <> '')) do
  begin
    I := Pos('.', V1);
    if I > 0 then
    begin
      S1 := Copy(V1, 1, I - 1);
      V1 := Copy(V1, I + 1, Length(V1));
    end
    else
    begin
      S1 := V1;
      V1 := '';
    end;

    I := Pos('.', V2);
    if I > 0 then
    begin
      S2 := Copy(V2, 1, I - 1);
      V2 := Copy(V2, I + 1, Length(V2));
    end
    else
    begin
      S2 := V2;
      V2 := '';
    end;

    N1 := StrToIntDef(S1, 0);
    N2 := StrToIntDef(S2, 0);

    if N1 > N2 then
      Result := 1
    else if N1 < N2 then
      Result := -1;
  end;
end;

// Read registry key from both 32-bit and 64-bit views
function RegQueryStringValueAll(RootKey: Integer; SubKeyName, ValueName: String; var Value: String): Boolean;
begin
  // First, try 64-bit view
  Result := RegQueryStringValue(RootKey, SubKeyName, ValueName, Value);
  // If not found, try 32-bit view (Wow6432Node)
  if not Result then
    Result := RegQueryStringValue(HKLM, 'Software\Wow6432Node\' + SubKeyName, ValueName, Value);
end;

function GetGamePath(AppID, RegKey, RegValue: String): String;
var
  InstallPath: String;
begin
  Result := '';
  // 1. Priority: Steam uninstall key (most reliable)
  if RegQueryStringValue(HKLM, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\Steam App ' + AppID, 'InstallLocation', InstallPath) then
  begin
    Result := InstallPath;
    Exit;
  end;

  // 2. SCS Software's own registry key (non-Steam or older)
  if RegQueryStringValueAll(HKLM, RegKey, RegValue, InstallPath) then
  begin
    Result := InstallPath;
    Exit;
  end;
end;


function InitializeSetup(): Boolean;
var
  InstalledVersion: String;
begin
  Result := True;
  // Check for existing installation version
  if RegQueryStringValue(HKLM, 'Software\SPF_Framework', 'Version', InstalledVersion) then
  begin
    Log(Format(CustomMessage('FoundInstalledVersionLog'), [InstalledVersion]));
    Log(Format(CustomMessage('ThisInstallerVersionLog'), ['{#MyAppVersion}']));
    
    // Compare versions
    if CompareVersion(InstalledVersion, '{#MyAppVersion}') > 0 then
    begin
      if MsgBox(Format(CustomMessage('DowngradeWarning'), [InstalledVersion, '{#MyAppVersion}']), mbConfirmation, MB_YESNO) = IDNO then
      begin
        Result := False;
        Exit;
      end;
    end;
  end;

  // Find game paths
  ATSPath := GetGamePath('{#MySteamAppIdATS}', 'Software\SCS Software\American Truck Simulator', 'Install');
  ETS2Path := GetGamePath('{#MySteamAppIdETS2}', 'Software\SCS Software\Euro Truck Simulator 2', 'Install');
  
  // If no paths found, the edit fields will be empty on the custom page
end;

procedure InitializeWizard();
begin
  if (ATSPath = '') then
  begin
    // We don't disable the task, just leave the path empty for the user to fill in
  end;
  if (ETS2Path = '') then
  begin
    // We don't disable the task, just leave the path empty
  end;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var
  Path: String;
  IsATS, IsETS2: Boolean;
begin
  Result := True;
  if CurPageID = wpSelectTasks then
  begin
    // Create the page dynamically after task selection
    CreatePathEditorPage();
  end
  else if (PathsPage <> nil) and (CurPageID = PathsPage.ID) then
  begin
    IsATS := WizardIsTaskSelected('InstallForATS');
    IsETS2 := WizardIsTaskSelected('InstallForETS2');

    if IsATS then
    begin
      Path := ATSPathEdit.Text;
      if not DirExists(Path + '\bin\win_x64') then
      begin
        MsgBox(CustomMessage('InvalidATSPathMsg'), mbError, MB_OK);
        Result := False;
        Exit;
      end;
      ATSPath := Path; // Update global variable with potentially edited path
    end;

    if IsETS2 then
    begin
      Path := ETS2PathEdit.Text;
      if not DirExists(Path + '\bin\win_x64') then
      begin
        MsgBox(CustomMessage('InvalidETS2PathMsg'), mbError, MB_OK);
        Result := False;
        Exit;
      end;
      ETS2Path := Path; // Update global variable
    end;
  end;
end;

function IsGameSelected(Game: String): Boolean;
begin
  if Uppercase(Game) = 'ATS' then
    Result := WizardIsTaskSelected('InstallForATS')
  else if Uppercase(Game) = 'ETS2' then
    Result := WizardIsTaskSelected('InstallForETS2')
  else
    Result := False;
end;

function GetInstallDir(Game: String): String;
begin
  if Uppercase(Game) = 'ATS' then
    Result := ATSPath
  else if Uppercase(Game) = 'ETS2' then
    Result := ETS2Path;
end;

function UpdateReadyMemo(Space, NewLine, MemoUserInfoInfo, MemoDirInfo, MemoTypeInfo, MemoComponentsInfo, MemoGroupInfo, MemoTasksInfo: String): String;
var
  SelectedTasks: String;
begin
  Result := MemoUserInfoInfo + NewLine + NewLine;
  SelectedTasks := '';

  if WizardIsTaskSelected('InstallForATS') then
    SelectedTasks := SelectedTasks + '  ' + CustomMessage('TaskInstallForATS') + ':' + NewLine + '    ' + ATSPath + NewLine;
  if WizardIsTaskSelected('InstallForETS2') then
    SelectedTasks := SelectedTasks + '  ' + CustomMessage('TaskInstallForETS2') + ':' + NewLine + '    ' + ETS2Path + NewLine;

  if Trim(SelectedTasks) <> '' then
  begin
    Result := Result + CustomMessage('InstallLocationsIntro') + NewLine + NewLine + SelectedTasks;
  end
  else
  begin
      Result := Result + CustomMessage('NoGameSelectedMsg') + NewLine;
  end;
end;
