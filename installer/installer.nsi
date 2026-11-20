; -- SPF Framework Installer Script (NSIS) --
; Replicates the behavior of the original Inno Setup installer (installer.iss).

; ---------------------------------------------------------------------------
; Defines
; ---------------------------------------------------------------------------
!define APP_NAME "SPF Framework"
!define APP_VERSION "1.2.2"
!define APP_PUBLISHER "TrackAndTruckDevs"
!define APP_URL "https://github.com/TrackAndTruckDevs/SPF-Framework"
!define STEAM_APPID_ATS "270880"
!define STEAM_APPID_ETS2 "227300"
!define REG_KEY "Software\SPF_Framework"
!define LEGACY_UNINSTALL_KEY "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{11a9f0ff-558b-4844-8804-7e9e2328ba27}_is1"
!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
!include FileFunc.nsh

; ---------------------------------------------------------------------------
; Split strings - extract the Nth quoted token from a VDF line.
; Adapted from geode-sdk/geode installer (NSIS wiki "Split strings").
; Usage: !insertmacro GET_STRING_TOKEN <input> <part>  ->  leaves result on stack
;   part 1 = first "quoted token", part 2 = second, etc.
; ---------------------------------------------------------------------------
!macro GET_STRING_TOKEN INPUT PART
  Push $R0
  Push $R1
  Push $R2
  StrCpy $R0 -1
  IntOp $R1 ${PART} * 2
  IntOp $R1 $R1 - 1
  findStart_loop_${PART}:
    IntOp $R0 $R0 + 1
    StrCpy $R2 ${INPUT} 1 $R0
    StrCmp $R2 "" error_${PART}
    StrCmp $R2 '"' 0 findStart_loop_${PART}
    IntOp $R1 $R1 - 1
    IntCmp $R1 0 0 0 findStart_loop_${PART}
    IntOp $R1 $R0 + 1
  findEnd_loop_${PART}:
    IntOp $R0 $R0 + 1
    StrCpy $R2 ${INPUT} 1 $R0
    StrCmp $R2 "" error_${PART}
    StrCmp $R2 '"' 0 findEnd_loop_${PART}
    IntOp $R0 $R0 - $R1
    StrCpy $R0 ${INPUT} $R0 $R1
    Goto done_${PART}
  error_${PART}:
    StrCpy $R0 "error"
  done_${PART}:
    Pop $R2
    Pop $R1
    Exch $R0
!macroend

; ---------------------------------------------------------------------------
; Compiler settings
; ---------------------------------------------------------------------------
Unicode true
SetCompressor /SOLID lzma
RequestExecutionLevel user

Name "${APP_NAME} ${APP_VERSION}"
OutFile "Output\spf-framework.exe"

; Version resource (file properties) — single source: APP_VERSION above
VIProductVersion "${APP_VERSION}.0"
VIAddVersionKey "FileVersion" "${APP_VERSION}.0"
VIAddVersionKey "ProductName" "${APP_NAME}"
VIAddVersionKey "FileDescription" "${APP_NAME} Installer"
VIAddVersionKey "ProductVersion" "${APP_VERSION}"
VIAddVersionKey "LegalCopyright" "Copyright (C) 2026 ${APP_PUBLISHER}"
VIAddVersionKey "OriginalFilename" "spf-framework.exe"
InstallDir "$LOCALAPPDATA\Programs\${APP_NAME}"
InstallDirRegKey HKCU "${REG_KEY}" "InstallDir"
ShowInstDetails show
ShowUnInstDetails show

; ---------------------------------------------------------------------------
; Modern UI 2
; ---------------------------------------------------------------------------
!include "MUI2.nsh"
!include "WinMessages.nsh"
!include "FileFunc.nsh"
!include "WordFunc.nsh"
!insertmacro VersionCompare
!include "nsDialogs.nsh"
!include "LogicLib.nsh"
!include "Sections.nsh"

; ---------------------------------------------------------------------------
; Global variables
; ---------------------------------------------------------------------------
Var ATSPath
Var ETS2Path
Var ATSPathEdit
Var ETS2PathEdit
Var DLLPathATS
Var DLLPathETS2
Var UnChkPlugins
Var UnChkSettings
Var UnDoPlugins
Var UnDoSettings

; ---------------------------------------------------------------------------
; Modern UI settings
; ---------------------------------------------------------------------------
!define MUI_ABORTWARNING
!define MUI_ICON "icon.ico"
!define MUI_UNICON "icon.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "150×57_minimal.bmp"
!define MUI_HEADERIMAGE_UNBITMAP "150×57_minimal.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP "164×164_full.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "164×164_full.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP_NOSTRETCH
!define MUI_UNWELCOMEFINISHPAGE_BITMAP_NOSTRETCH
!define MUI_WELCOMEPAGE_TITLE_3LINES
!define MUI_FINISHPAGE_TITLE_3LINES
!define MUI_WELCOMEPAGE_TEXT "$(IDS_WELCOME_TEXT)"

; Language selection dialog (LangDLL): show every launch, all languages,
; remember choice for installer and uninstaller
!define MUI_LANGDLL_REGISTRY_ROOT HKCU
!define MUI_LANGDLL_REGISTRY_KEY "${REG_KEY}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "Language"
!define MUI_LANGDLL_WINDOWTITLE "$(IDS_LANG_TITLE)"
!define MUI_LANGDLL_INFO "$(IDS_LANG_INFO)"
!define MUI_LANGDLL_ALLLANGUAGES
!define MUI_LANGDLL_ALWAYSSHOW

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
Page custom PageConfirmPaths PageConfirmPathsLeave
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

UninstPage custom un.ConfirmPage
!insertmacro MUI_UNPAGE_INSTFILES

; ---------------------------------------------------------------------------
; Languages (matching the Inno set where available in NSIS)
; ---------------------------------------------------------------------------
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Ukrainian"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "Polish"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "Portuguese"
!insertmacro MUI_LANGUAGE "Italian"
!insertmacro MUI_LANGUAGE "Dutch"
!insertmacro MUI_LANGUAGE "Turkish"
!insertmacro MUI_LANGUAGE "Czech"
!insertmacro MUI_LANGUAGE "Hungarian"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "Korean"
!insertmacro MUI_LANGUAGE "Norwegian"
!insertmacro MUI_LANGUAGE "Danish"
!insertmacro MUI_LANGUAGE "Finnish"

; Language selection dialog must be stored first for solid compression
!insertmacro MUI_RESERVEFILE_LANGDLL

; --- Localized strings: path page, language dialog, section names ---
LangString IDS_LANG_TITLE ${LANG_ENGLISH} "Select Language"
LangString IDS_LANG_TITLE ${LANG_UKRAINIAN} "Вибір мови"
LangString IDS_LANG_TITLE ${LANG_GERMAN} "Sprache auswählen"
LangString IDS_LANG_TITLE ${LANG_FRENCH} "Choisir la langue"
LangString IDS_LANG_TITLE ${LANG_SPANISH} "Seleccionar idioma"
LangString IDS_LANG_TITLE ${LANG_POLISH} "Wybierz język"
LangString IDS_LANG_TITLE ${LANG_RUSSIAN} "Выберите язык"
LangString IDS_LANG_TITLE ${LANG_PORTUGUESEBR} "Selecionar idioma"
LangString IDS_LANG_TITLE ${LANG_PORTUGUESE} "Selecionar idioma"
LangString IDS_LANG_TITLE ${LANG_ITALIAN} "Seleziona lingua"
LangString IDS_LANG_TITLE ${LANG_DUTCH} "Taal selecteren"
LangString IDS_LANG_TITLE ${LANG_TURKISH} "Dil seçin"
LangString IDS_LANG_TITLE ${LANG_CZECH} "Vyberte jazyk"
LangString IDS_LANG_TITLE ${LANG_HUNGARIAN} "Nyelv kiválasztása"
LangString IDS_LANG_TITLE ${LANG_SIMPCHINESE} "选择语言"
LangString IDS_LANG_TITLE ${LANG_JAPANESE} "言語を選択"
LangString IDS_LANG_TITLE ${LANG_KOREAN} "언어 선택"
LangString IDS_LANG_TITLE ${LANG_NORWEGIAN} "Velg språk"
LangString IDS_LANG_TITLE ${LANG_DANISH} "Vælg sprog"
LangString IDS_LANG_TITLE ${LANG_FINNISH} "Valitse kieli"

LangString IDS_LANG_INFO ${LANG_ENGLISH} "Please select a language."
LangString IDS_LANG_INFO ${LANG_UKRAINIAN} "Будь ласка, виберіть мову."
LangString IDS_LANG_INFO ${LANG_GERMAN} "Bitte wählen Sie eine Sprache aus."
LangString IDS_LANG_INFO ${LANG_FRENCH} "Veuillez sélectionner une langue."
LangString IDS_LANG_INFO ${LANG_SPANISH} "Seleccione un idioma."
LangString IDS_LANG_INFO ${LANG_POLISH} "Wybierz język."
LangString IDS_LANG_INFO ${LANG_RUSSIAN} "Пожалуйста, выберите язык."
LangString IDS_LANG_INFO ${LANG_PORTUGUESEBR} "Selecione um idioma."
LangString IDS_LANG_INFO ${LANG_PORTUGUESE} "Selecione um idioma."
LangString IDS_LANG_INFO ${LANG_ITALIAN} "Seleziona una lingua."
LangString IDS_LANG_INFO ${LANG_DUTCH} "Selecteer een taal."
LangString IDS_LANG_INFO ${LANG_TURKISH} "Lütfen bir dil seçin."
LangString IDS_LANG_INFO ${LANG_CZECH} "Vyberte prosím jazyk."
LangString IDS_LANG_INFO ${LANG_HUNGARIAN} "Válasszon nyelvet."
LangString IDS_LANG_INFO ${LANG_SIMPCHINESE} "请选择语言。"
LangString IDS_LANG_INFO ${LANG_JAPANESE} "言語を選択してください。"
LangString IDS_LANG_INFO ${LANG_KOREAN} "언어를 선택하세요."
LangString IDS_LANG_INFO ${LANG_NORWEGIAN} "Velg et språk."
LangString IDS_LANG_INFO ${LANG_DANISH} "Vælg et sprog."
LangString IDS_LANG_INFO ${LANG_FINNISH} "Valitse kieli."

LangString IDS_PATHS_TITLE ${LANG_ENGLISH} "Installation paths"
LangString IDS_PATHS_TITLE ${LANG_UKRAINIAN} "Шляхи встановлення"
LangString IDS_PATHS_TITLE ${LANG_GERMAN} "Installationspfade"
LangString IDS_PATHS_TITLE ${LANG_FRENCH} "Chemins d'installation"
LangString IDS_PATHS_TITLE ${LANG_SPANISH} "Rutas de instalación"
LangString IDS_PATHS_TITLE ${LANG_POLISH} "Ścieżki instalacji"
LangString IDS_PATHS_TITLE ${LANG_RUSSIAN} "Пути установки"
LangString IDS_PATHS_TITLE ${LANG_PORTUGUESEBR} "Caminhos de instalação"
LangString IDS_PATHS_TITLE ${LANG_PORTUGUESE} "Caminhos de instalação"
LangString IDS_PATHS_TITLE ${LANG_ITALIAN} "Percorsi di installazione"
LangString IDS_PATHS_TITLE ${LANG_DUTCH} "Installatiepaden"
LangString IDS_PATHS_TITLE ${LANG_TURKISH} "Kurulum yolları"
LangString IDS_PATHS_TITLE ${LANG_CZECH} "Cesty instalace"
LangString IDS_PATHS_TITLE ${LANG_HUNGARIAN} "Telepítési elérési utak"
LangString IDS_PATHS_TITLE ${LANG_SIMPCHINESE} "安装路径"
LangString IDS_PATHS_TITLE ${LANG_JAPANESE} "インストールパス"
LangString IDS_PATHS_TITLE ${LANG_KOREAN} "설치 경로"
LangString IDS_PATHS_TITLE ${LANG_NORWEGIAN} "Installeringsbaner"
LangString IDS_PATHS_TITLE ${LANG_DANISH} "Installationsstier"
LangString IDS_PATHS_TITLE ${LANG_FINNISH} "Asennuspolut"

LangString IDS_PATHS_DESC ${LANG_ENGLISH} "Please confirm the installation paths for the selected games."
LangString IDS_PATHS_DESC ${LANG_UKRAINIAN} "Підтвердьте шляхи встановлення для вибраних ігор."
LangString IDS_PATHS_DESC ${LANG_GERMAN} "Bitte bestätigen Sie die Installationspfade der ausgewählten Spiele."
LangString IDS_PATHS_DESC ${LANG_FRENCH} "Veuillez confirmer les chemins d'installation des jeux sélectionnés."
LangString IDS_PATHS_DESC ${LANG_SPANISH} "Confirme las rutas de instalación de los juegos seleccionados."
LangString IDS_PATHS_DESC ${LANG_POLISH} "Potwierdź ścieżki instalacji wybranych gier."
LangString IDS_PATHS_DESC ${LANG_RUSSIAN} "Подтвердите пути установки для выбранных игр."
LangString IDS_PATHS_DESC ${LANG_PORTUGUESEBR} "Confirme os caminhos de instalação dos jogos selecionados."
LangString IDS_PATHS_DESC ${LANG_PORTUGUESE} "Confirme os caminhos de instalação dos jogos selecionados."
LangString IDS_PATHS_DESC ${LANG_ITALIAN} "Conferma i percorsi di installazione dei giochi selezionati."
LangString IDS_PATHS_DESC ${LANG_DUTCH} "Bevestig de installatiepaden van de geselecteerde spellen."
LangString IDS_PATHS_DESC ${LANG_TURKISH} "Seçilen oyunların kurulum yollarını onaylayın."
LangString IDS_PATHS_DESC ${LANG_CZECH} "Potvrďte cesty instalace vybraných her."
LangString IDS_PATHS_DESC ${LANG_HUNGARIAN} "Erősítse meg a kiválasztott játékok telepítési elérési utait."
LangString IDS_PATHS_DESC ${LANG_SIMPCHINESE} "请确认所选游戏的安装路径。"
LangString IDS_PATHS_DESC ${LANG_JAPANESE} "選択したゲームのインストールパスを確認してください。"
LangString IDS_PATHS_DESC ${LANG_KOREAN} "선택한 게임의 설치 경로를 확인하세요."
LangString IDS_PATHS_DESC ${LANG_NORWEGIAN} "Bekreft installasjonsbanene for de valgte spillene."
LangString IDS_PATHS_DESC ${LANG_DANISH} "Bekræft installationsstierne for de valgte spil."
LangString IDS_PATHS_DESC ${LANG_FINNISH} "Vahvista valittujen pelien asennuspolut."

LangString IDS_ATS_PATH ${LANG_ENGLISH} "American Truck Simulator Path:"
LangString IDS_ATS_PATH ${LANG_UKRAINIAN} "Шлях до American Truck Simulator:"
LangString IDS_ATS_PATH ${LANG_GERMAN} "American Truck Simulator-Pfad:"
LangString IDS_ATS_PATH ${LANG_FRENCH} "Chemin d'American Truck Simulator :"
LangString IDS_ATS_PATH ${LANG_SPANISH} "Ruta de American Truck Simulator:"
LangString IDS_ATS_PATH ${LANG_POLISH} "Ścieżka do American Truck Simulator:"
LangString IDS_ATS_PATH ${LANG_RUSSIAN} "Путь к American Truck Simulator:"
LangString IDS_ATS_PATH ${LANG_PORTUGUESEBR} "Caminho do American Truck Simulator:"
LangString IDS_ATS_PATH ${LANG_PORTUGUESE} "Caminho do American Truck Simulator:"
LangString IDS_ATS_PATH ${LANG_ITALIAN} "Percorso di American Truck Simulator:"
LangString IDS_ATS_PATH ${LANG_DUTCH} "Pad naar American Truck Simulator:"
LangString IDS_ATS_PATH ${LANG_TURKISH} "American Truck Simulator yolu:"
LangString IDS_ATS_PATH ${LANG_CZECH} "Cesta k American Truck Simulator:"
LangString IDS_ATS_PATH ${LANG_HUNGARIAN} "American Truck Simulator elérési útja:"
LangString IDS_ATS_PATH ${LANG_SIMPCHINESE} "American Truck Simulator 路径:"
LangString IDS_ATS_PATH ${LANG_JAPANESE} "American Truck Simulator のパス:"
LangString IDS_ATS_PATH ${LANG_KOREAN} "American Truck Simulator 경로:"
LangString IDS_ATS_PATH ${LANG_NORWEGIAN} "American Truck Simulator-sti:"
LangString IDS_ATS_PATH ${LANG_DANISH} "American Truck Simulator-sti:"
LangString IDS_ATS_PATH ${LANG_FINNISH} "American Truck Simulator -polku:"

LangString IDS_ETS2_PATH ${LANG_ENGLISH} "Euro Truck Simulator 2 Path:"
LangString IDS_ETS2_PATH ${LANG_UKRAINIAN} "Шлях до Euro Truck Simulator 2:"
LangString IDS_ETS2_PATH ${LANG_GERMAN} "Euro Truck Simulator 2-Pfad:"
LangString IDS_ETS2_PATH ${LANG_FRENCH} "Chemin d'Euro Truck Simulator 2 :"
LangString IDS_ETS2_PATH ${LANG_SPANISH} "Ruta de Euro Truck Simulator 2:"
LangString IDS_ETS2_PATH ${LANG_POLISH} "Ścieżka do Euro Truck Simulator 2:"
LangString IDS_ETS2_PATH ${LANG_RUSSIAN} "Путь к Euro Truck Simulator 2:"
LangString IDS_ETS2_PATH ${LANG_PORTUGUESEBR} "Caminho do Euro Truck Simulator 2:"
LangString IDS_ETS2_PATH ${LANG_PORTUGUESE} "Caminho do Euro Truck Simulator 2:"
LangString IDS_ETS2_PATH ${LANG_ITALIAN} "Percorso di Euro Truck Simulator 2:"
LangString IDS_ETS2_PATH ${LANG_DUTCH} "Pad naar Euro Truck Simulator 2:"
LangString IDS_ETS2_PATH ${LANG_TURKISH} "Euro Truck Simulator 2 yolu:"
LangString IDS_ETS2_PATH ${LANG_CZECH} "Cesta k Euro Truck Simulator 2:"
LangString IDS_ETS2_PATH ${LANG_HUNGARIAN} "Euro Truck Simulator 2 elérési útja:"
LangString IDS_ETS2_PATH ${LANG_SIMPCHINESE} "Euro Truck Simulator 2 路径:"
LangString IDS_ETS2_PATH ${LANG_JAPANESE} "Euro Truck Simulator 2 のパス:"
LangString IDS_ETS2_PATH ${LANG_KOREAN} "Euro Truck Simulator 2 경로:"
LangString IDS_ETS2_PATH ${LANG_NORWEGIAN} "Euro Truck Simulator 2-sti:"
LangString IDS_ETS2_PATH ${LANG_DANISH} "Euro Truck Simulator 2-sti:"
LangString IDS_ETS2_PATH ${LANG_FINNISH} "Euro Truck Simulator 2 -polku:"

LangString IDS_BROWSE ${LANG_ENGLISH} "Browse..."
LangString IDS_BROWSE ${LANG_UKRAINIAN} "Огляд..."
LangString IDS_BROWSE ${LANG_GERMAN} "Durchsuchen..."
LangString IDS_BROWSE ${LANG_FRENCH} "Parcourir..."
LangString IDS_BROWSE ${LANG_SPANISH} "Examinar..."
LangString IDS_BROWSE ${LANG_POLISH} "Przeglądaj..."
LangString IDS_BROWSE ${LANG_RUSSIAN} "Обзор..."
LangString IDS_BROWSE ${LANG_PORTUGUESEBR} "Procurar..."
LangString IDS_BROWSE ${LANG_PORTUGUESE} "Procurar..."
LangString IDS_BROWSE ${LANG_ITALIAN} "Sfoglia..."
LangString IDS_BROWSE ${LANG_DUTCH} "Bladeren..."
LangString IDS_BROWSE ${LANG_TURKISH} "Gözat..."
LangString IDS_BROWSE ${LANG_CZECH} "Procházet..."
LangString IDS_BROWSE ${LANG_HUNGARIAN} "Tallózás..."
LangString IDS_BROWSE ${LANG_SIMPCHINESE} "浏览..."
LangString IDS_BROWSE ${LANG_JAPANESE} "参照..."
LangString IDS_BROWSE ${LANG_KOREAN} "찾아보기..."
LangString IDS_BROWSE ${LANG_NORWEGIAN} "Bla..."
LangString IDS_BROWSE ${LANG_DANISH} "Gennemse..."
LangString IDS_BROWSE ${LANG_FINNISH} "Selaa..."

LangString IDS_SELECT_ATS ${LANG_ENGLISH} "Select American Truck Simulator folder"
LangString IDS_SELECT_ATS ${LANG_UKRAINIAN} "Виберіть папку American Truck Simulator"
LangString IDS_SELECT_ATS ${LANG_GERMAN} "American Truck Simulator-Ordner auswählen"
LangString IDS_SELECT_ATS ${LANG_FRENCH} "Sélectionnez le dossier American Truck Simulator"
LangString IDS_SELECT_ATS ${LANG_SPANISH} "Seleccione la carpeta de American Truck Simulator"
LangString IDS_SELECT_ATS ${LANG_POLISH} "Wybierz folder American Truck Simulator"
LangString IDS_SELECT_ATS ${LANG_RUSSIAN} "Выберите папку American Truck Simulator"
LangString IDS_SELECT_ATS ${LANG_PORTUGUESEBR} "Selecione a pasta do American Truck Simulator"
LangString IDS_SELECT_ATS ${LANG_PORTUGUESE} "Selecione a pasta do American Truck Simulator"
LangString IDS_SELECT_ATS ${LANG_ITALIAN} "Seleziona la cartella di American Truck Simulator"
LangString IDS_SELECT_ATS ${LANG_DUTCH} "Selecteer de map American Truck Simulator"
LangString IDS_SELECT_ATS ${LANG_TURKISH} "American Truck Simulator klasörünü seçin"
LangString IDS_SELECT_ATS ${LANG_CZECH} "Vyberte složku American Truck Simulator"
LangString IDS_SELECT_ATS ${LANG_HUNGARIAN} "Válassza ki az American Truck Simulator mappáját"
LangString IDS_SELECT_ATS ${LANG_SIMPCHINESE} "选择 American Truck Simulator 文件夹"
LangString IDS_SELECT_ATS ${LANG_JAPANESE} "American Truck Simulator フォルダーを選択"
LangString IDS_SELECT_ATS ${LANG_KOREAN} "American Truck Simulator 폴더 선택"
LangString IDS_SELECT_ATS ${LANG_NORWEGIAN} "Velg mappen for American Truck Simulator"
LangString IDS_SELECT_ATS ${LANG_DANISH} "Vælg mappen til American Truck Simulator"
LangString IDS_SELECT_ATS ${LANG_FINNISH} "Valitse American Truck Simulator -kansio"

LangString IDS_SELECT_ETS2 ${LANG_ENGLISH} "Select Euro Truck Simulator 2 folder"
LangString IDS_SELECT_ETS2 ${LANG_UKRAINIAN} "Виберіть папку Euro Truck Simulator 2"
LangString IDS_SELECT_ETS2 ${LANG_GERMAN} "Euro Truck Simulator 2-Ordner auswählen"
LangString IDS_SELECT_ETS2 ${LANG_FRENCH} "Sélectionnez le dossier Euro Truck Simulator 2"
LangString IDS_SELECT_ETS2 ${LANG_SPANISH} "Seleccione la carpeta de Euro Truck Simulator 2"
LangString IDS_SELECT_ETS2 ${LANG_POLISH} "Wybierz folder Euro Truck Simulator 2"
LangString IDS_SELECT_ETS2 ${LANG_RUSSIAN} "Выберите папку Euro Truck Simulator 2"
LangString IDS_SELECT_ETS2 ${LANG_PORTUGUESEBR} "Selecione a pasta do Euro Truck Simulator 2"
LangString IDS_SELECT_ETS2 ${LANG_PORTUGUESE} "Selecione a pasta do Euro Truck Simulator 2"
LangString IDS_SELECT_ETS2 ${LANG_ITALIAN} "Seleziona la cartella di Euro Truck Simulator 2"
LangString IDS_SELECT_ETS2 ${LANG_DUTCH} "Selecteer de map Euro Truck Simulator 2"
LangString IDS_SELECT_ETS2 ${LANG_TURKISH} "Euro Truck Simulator 2 klasörünü seçin"
LangString IDS_SELECT_ETS2 ${LANG_CZECH} "Vyberte složku Euro Truck Simulator 2"
LangString IDS_SELECT_ETS2 ${LANG_HUNGARIAN} "Válassza ki a Euro Truck Simulator 2 mappáját"
LangString IDS_SELECT_ETS2 ${LANG_SIMPCHINESE} "选择 Euro Truck Simulator 2 文件夹"
LangString IDS_SELECT_ETS2 ${LANG_JAPANESE} "Euro Truck Simulator 2 フォルダーを選択"
LangString IDS_SELECT_ETS2 ${LANG_KOREAN} "Euro Truck Simulator 2 폴더 선택"
LangString IDS_SELECT_ETS2 ${LANG_NORWEGIAN} "Velg mappen for Euro Truck Simulator 2"
LangString IDS_SELECT_ETS2 ${LANG_DANISH} "Vælg mappen til Euro Truck Simulator 2"
LangString IDS_SELECT_ETS2 ${LANG_FINNISH} "Valitse Euro Truck Simulator 2 -kansio"

LangString IDS_INVALID_ATS ${LANG_ENGLISH} "The selected directory for American Truck Simulator is not valid. Please select the root folder of the game."
LangString IDS_INVALID_ATS ${LANG_UKRAINIAN} "Вибрана директорія для American Truck Simulator недійсна. Виберіть кореневу папку гри."
LangString IDS_INVALID_ATS ${LANG_GERMAN} "Das ausgewählte Verzeichnis für American Truck Simulator ist ungültig. Bitte wählen Sie den Stammordner des Spiels aus."
LangString IDS_INVALID_ATS ${LANG_FRENCH} "Le dossier sélectionné pour American Truck Simulator n'est pas valide. Veuillez sélectionner le dossier racine du jeu."
LangString IDS_INVALID_ATS ${LANG_SPANISH} "El directorio seleccionado para American Truck Simulator no es válido. Seleccione la carpeta raíz del juego."
LangString IDS_INVALID_ATS ${LANG_POLISH} "Wybrany katalog dla American Truck Simulator jest nieprawidłowy. Wybierz folder główny gry."
LangString IDS_INVALID_ATS ${LANG_RUSSIAN} "Выбранная директория для American Truck Simulator недопустима. Выберите корневую папку игры."
LangString IDS_INVALID_ATS ${LANG_PORTUGUESEBR} "O diretório selecionado para American Truck Simulator não é válido. Selecione a pasta raiz do jogo."
LangString IDS_INVALID_ATS ${LANG_PORTUGUESE} "O diretório selecionado para American Truck Simulator não é válido. Selecione a pasta raiz do jogo."
LangString IDS_INVALID_ATS ${LANG_ITALIAN} "La directory selezionata per American Truck Simulator non è valida. Seleziona la cartella root del gioco."
LangString IDS_INVALID_ATS ${LANG_DUTCH} "De geselecteerde map voor American Truck Simulator is ongeldig. Selecteer de hoofdmap van het spel."
LangString IDS_INVALID_ATS ${LANG_TURKISH} "American Truck Simulator için seçilen dizin geçersiz. Lütfen oyunun kök klasörünü seçin."
LangString IDS_INVALID_ATS ${LANG_CZECH} "Vybraný adresář pro American Truck Simulator není platný. Vyberte kořenovou složku hry."
LangString IDS_INVALID_ATS ${LANG_HUNGARIAN} "A kiválasztott American Truck Simulator könyvtár érvénytelen. Válassza ki a játék gyöker mappáját."
LangString IDS_INVALID_ATS ${LANG_SIMPCHINESE} "所选的 American Truck Simulator 目录无效。请选择游戏的根文件夹。"
LangString IDS_INVALID_ATS ${LANG_JAPANESE} "選択された American Truck Simulator のディレクトリは無効です。ゲームのルートフォルダーを選択してください。"
LangString IDS_INVALID_ATS ${LANG_KOREAN} "선택한 American Truck Simulator 디렉터리가 잘못되었습니다. 게임의 루트 폴더를 선택하세요."
LangString IDS_INVALID_ATS ${LANG_NORWEGIAN} "Valgt mappe for American Truck Simulator er ugyldig. Velg spillets rotmappe."
LangString IDS_INVALID_ATS ${LANG_DANISH} "Den valgte mappe til American Truck Simulator er ugyldig. Vælg spillets rodmappe."
LangString IDS_INVALID_ATS ${LANG_FINNISH} "Valittu American Truck Simulator -hakemisto on virheellinen. Valitse pelen juurikansio."

LangString IDS_INVALID_ETS2 ${LANG_ENGLISH} "The selected directory for Euro Truck Simulator 2 is not valid. Please select the root folder of the game."
LangString IDS_INVALID_ETS2 ${LANG_UKRAINIAN} "Вибрана директорія для Euro Truck Simulator 2 недійсна. Виберіть кореневу папку гри."
LangString IDS_INVALID_ETS2 ${LANG_GERMAN} "Das ausgewählte Verzeichnis für Euro Truck Simulator 2 ist ungültig. Bitte wählen Sie den Stammordner des Spiels aus."
LangString IDS_INVALID_ETS2 ${LANG_FRENCH} "Le dossier sélectionné pour Euro Truck Simulator 2 n'est pas valide. Veuillez sélectionner le dossier racine du jeu."
LangString IDS_INVALID_ETS2 ${LANG_SPANISH} "El directorio seleccionado para Euro Truck Simulator 2 no es válido. Seleccione la carpeta raíz del juego."
LangString IDS_INVALID_ETS2 ${LANG_POLISH} "Wybrany katalog dla Euro Truck Simulator 2 jest nieprawidłowy. Wybierz folder główny gry."
LangString IDS_INVALID_ETS2 ${LANG_RUSSIAN} "Выбранная директория для Euro Truck Simulator 2 недопустима. Выберите корневую папку игры."
LangString IDS_INVALID_ETS2 ${LANG_PORTUGUESEBR} "O diretório selecionado para Euro Truck Simulator 2 não é válido. Selecione a pasta raiz do jogo."
LangString IDS_INVALID_ETS2 ${LANG_PORTUGUESE} "O diretório selecionado para Euro Truck Simulator 2 não é válido. Selecione a pasta raiz do jogo."
LangString IDS_INVALID_ETS2 ${LANG_ITALIAN} "La directory selezionata per Euro Truck Simulator 2 non è valida. Seleziona la cartella root del gioco."
LangString IDS_INVALID_ETS2 ${LANG_DUTCH} "De geselecteerde map voor Euro Truck Simulator 2 is ongeldig. Selecteer de hoofdmap van het spel."
LangString IDS_INVALID_ETS2 ${LANG_TURKISH} "Euro Truck Simulator 2 için seçilen dizin geçersiz. Lütfen oyunun kök klasörünü seçin."
LangString IDS_INVALID_ETS2 ${LANG_CZECH} "Vybraný adresář pro Euro Truck Simulator 2 není platný. Vyberte kořenovou složku hry."
LangString IDS_INVALID_ETS2 ${LANG_HUNGARIAN} "A kiválasztott Euro Truck Simulator 2 könyvtár érvénytelen. Válassza ki a játék gyöker mappáját."
LangString IDS_INVALID_ETS2 ${LANG_SIMPCHINESE} "所选的 Euro Truck Simulator 2 目录无效。请选择游戏的根文件夹。"
LangString IDS_INVALID_ETS2 ${LANG_JAPANESE} "選択された Euro Truck Simulator 2 のディレクトリは無効です。ゲームのルートフォルダーを選択してください。"
LangString IDS_INVALID_ETS2 ${LANG_KOREAN} "선택한 Euro Truck Simulator 2 디렉터리가 잘못되었습니다. 게임의 루트 폴더를 선택하세요."
LangString IDS_INVALID_ETS2 ${LANG_NORWEGIAN} "Valgt mappe for Euro Truck Simulator 2 er ugyldig. Velg spillets rotmappe."
LangString IDS_INVALID_ETS2 ${LANG_DANISH} "Den valgte mappe til Euro Truck Simulator 2 er ugyldig. Vælg spillets rodmappe."
LangString IDS_INVALID_ETS2 ${LANG_FINNISH} "Valittu Euro Truck Simulator 2 -hakemisto on virheellinen. Valitse pelen juurikansio."

LangString IDS_SEC_GAMES ${LANG_ENGLISH} "Games"
LangString IDS_SEC_GAMES ${LANG_UKRAINIAN} "Ігри"
LangString IDS_SEC_GAMES ${LANG_GERMAN} "Spiele"
LangString IDS_SEC_GAMES ${LANG_FRENCH} "Jeux"
LangString IDS_SEC_GAMES ${LANG_SPANISH} "Juegos"
LangString IDS_SEC_GAMES ${LANG_POLISH} "Gry"
LangString IDS_SEC_GAMES ${LANG_RUSSIAN} "Игры"
LangString IDS_SEC_GAMES ${LANG_PORTUGUESEBR} "Jogos"
LangString IDS_SEC_GAMES ${LANG_PORTUGUESE} "Jogos"
LangString IDS_SEC_GAMES ${LANG_ITALIAN} "Giochi"
LangString IDS_SEC_GAMES ${LANG_DUTCH} "Games"
LangString IDS_SEC_GAMES ${LANG_TURKISH} "Oyunlar"
LangString IDS_SEC_GAMES ${LANG_CZECH} "Hry"
LangString IDS_SEC_GAMES ${LANG_HUNGARIAN} "Játékok"
LangString IDS_SEC_GAMES ${LANG_SIMPCHINESE} "游戏"
LangString IDS_SEC_GAMES ${LANG_JAPANESE} "ゲーム"
LangString IDS_SEC_GAMES ${LANG_KOREAN} "게임"
LangString IDS_SEC_GAMES ${LANG_NORWEGIAN} "Spill"
LangString IDS_SEC_GAMES ${LANG_DANISH} "Spil"
LangString IDS_SEC_GAMES ${LANG_FINNISH} "Pelit"

LangString IDS_SEC_SHORTCUT ${LANG_ENGLISH} "Create desktop shortcut for plugin folders"
LangString IDS_SEC_SHORTCUT ${LANG_UKRAINIAN} "Створити ярлики папок плагінів на робочому столі"
LangString IDS_SEC_SHORTCUT ${LANG_GERMAN} "Desktop-Verknüpfung für Plugin-Ordner erstellen"
LangString IDS_SEC_SHORTCUT ${LANG_FRENCH} "Créer un raccourci bureau pour les dossiers de plugins"
LangString IDS_SEC_SHORTCUT ${LANG_SPANISH} "Crear acceso directo en el escritorio para las carpetas de plugins"
LangString IDS_SEC_SHORTCUT ${LANG_POLISH} "Utwórz skrót na pulpicie do folderów wtyczek"
LangString IDS_SEC_SHORTCUT ${LANG_RUSSIAN} "Ярлык на рабочем столе к папкам плагинов"
LangString IDS_SEC_SHORTCUT ${LANG_PORTUGUESEBR} "Criar atalho na área de trabalho para as pastas de plugins"
LangString IDS_SEC_SHORTCUT ${LANG_PORTUGUESE} "Criar atalho na área de trabalho para as pastas de plugins"
LangString IDS_SEC_SHORTCUT ${LANG_ITALIAN} "Crea scorciatoia sul desktop per le cartelle dei plugin"
LangString IDS_SEC_SHORTCUT ${LANG_DUTCH} "Snelkoppeling op het bureaublad maken voor plugin-mappen"
LangString IDS_SEC_SHORTCUT ${LANG_TURKISH} "Eklenti klasörleri için masaüstü kısayolu oluştur"
LangString IDS_SEC_SHORTCUT ${LANG_CZECH} "Vytvořit zástupce na ploše pro složky zásuvných modulů"
LangString IDS_SEC_SHORTCUT ${LANG_HUNGARIAN} "Asztali parancsikon létrehozása a bővítmény mappákhoz"
LangString IDS_SEC_SHORTCUT ${LANG_SIMPCHINESE} "为插件文件夹创建桌面快捷方式"
LangString IDS_SEC_SHORTCUT ${LANG_JAPANESE} "プラグインフォルダーのデスクトップショートカットを作成"
LangString IDS_SEC_SHORTCUT ${LANG_KOREAN} "플러그인 폴더의 바탕화면 바로가기 만들기"
LangString IDS_SEC_SHORTCUT ${LANG_NORWEGIAN} "Opprett snarvei på skrivebordet for plugin-mapper"
LangString IDS_SEC_SHORTCUT ${LANG_DANISH} "Opret genvej på skrivebordet for plugin-mapper"
LangString IDS_SEC_SHORTCUT ${LANG_FINNISH} "Luo työpöydän pikakuvake lisäosakansioille"

LangString IDS_UNCONF_TITLE ${LANG_ENGLISH} "Uninstall SPF Framework"
LangString IDS_UNCONF_TITLE ${LANG_UKRAINIAN} "Видалення SPF Framework"
LangString IDS_UNCONF_TITLE ${LANG_GERMAN} "SPF Framework deinstallieren"
LangString IDS_UNCONF_TITLE ${LANG_FRENCH} "Désinstaller SPF Framework"
LangString IDS_UNCONF_TITLE ${LANG_SPANISH} "Desinstalar SPF Framework"
LangString IDS_UNCONF_TITLE ${LANG_POLISH} "Odinstaluj SPF Framework"
LangString IDS_UNCONF_TITLE ${LANG_RUSSIAN} "Удаление SPF Framework"
LangString IDS_UNCONF_TITLE ${LANG_PORTUGUESEBR} "Desinstalar SPF Framework"
LangString IDS_UNCONF_TITLE ${LANG_PORTUGUESE} "Desinstalar SPF Framework"
LangString IDS_UNCONF_TITLE ${LANG_ITALIAN} "Disinstalla SPF Framework"
LangString IDS_UNCONF_TITLE ${LANG_DUTCH} "SPF Framework verwijderen"
LangString IDS_UNCONF_TITLE ${LANG_TURKISH} "SPF Framework'u kaldır"
LangString IDS_UNCONF_TITLE ${LANG_CZECH} "Odinstalace SPF Framework"
LangString IDS_UNCONF_TITLE ${LANG_HUNGARIAN} "SPF Framework eltávolítása"
LangString IDS_UNCONF_TITLE ${LANG_SIMPCHINESE} "卸载 SPF Framework"
LangString IDS_UNCONF_TITLE ${LANG_JAPANESE} "SPF Framework のアンインストール"
LangString IDS_UNCONF_TITLE ${LANG_KOREAN} "SPF Framework 제거"
LangString IDS_UNCONF_TITLE ${LANG_NORWEGIAN} "Avinstaller SPF Framework"
LangString IDS_UNCONF_TITLE ${LANG_DANISH} "Afinstaller SPF Framework"
LangString IDS_UNCONF_TITLE ${LANG_FINNISH} "Poista SPF Framework"

LangString IDS_UNCONF_SUBTITLE ${LANG_ENGLISH} "Uninstall SPF Framework from your computer."
LangString IDS_UNCONF_SUBTITLE ${LANG_UKRAINIAN} "Видалення SPF Framework з вашого комп'ютера."
LangString IDS_UNCONF_SUBTITLE ${LANG_GERMAN} "SPF Framework von Ihrem Computer entfernen."
LangString IDS_UNCONF_SUBTITLE ${LANG_FRENCH} "Désinstaller SPF Framework de votre ordinateur."
LangString IDS_UNCONF_SUBTITLE ${LANG_SPANISH} "Desinstalar SPF Framework de su equipo."
LangString IDS_UNCONF_SUBTITLE ${LANG_POLISH} "Odinstaluj SPF Framework z komputera."
LangString IDS_UNCONF_SUBTITLE ${LANG_RUSSIAN} "Удаление SPF Framework с вашего компьютера."
LangString IDS_UNCONF_SUBTITLE ${LANG_PORTUGUESEBR} "Desinstalar SPF Framework do seu computador."
LangString IDS_UNCONF_SUBTITLE ${LANG_PORTUGUESE} "Desinstalar SPF Framework do seu computador."
LangString IDS_UNCONF_SUBTITLE ${LANG_ITALIAN} "Disinstalla SPF Framework dal tuo computer."
LangString IDS_UNCONF_SUBTITLE ${LANG_DUTCH} "SPF Framework van uw computer verwijderen."
LangString IDS_UNCONF_SUBTITLE ${LANG_TURKISH} "SPF Framework'u bilgisayarınızdan kaldırın."
LangString IDS_UNCONF_SUBTITLE ${LANG_CZECH} "Odinstalace SPF Framework z počítače."
LangString IDS_UNCONF_SUBTITLE ${LANG_HUNGARIAN} "SPF Framework eltávolítása a számítógépről."
LangString IDS_UNCONF_SUBTITLE ${LANG_SIMPCHINESE} "从您的计算机中卸载 SPF Framework。"
LangString IDS_UNCONF_SUBTITLE ${LANG_JAPANESE} "お使いのコンピューターから SPF Framework をアンインストールします。"
LangString IDS_UNCONF_SUBTITLE ${LANG_KOREAN} "컴퓨터에서 SPF Framework 제거."
LangString IDS_UNCONF_SUBTITLE ${LANG_NORWEGIAN} "Avinstaller SPF Framework fra datamaskinen."
LangString IDS_UNCONF_SUBTITLE ${LANG_DANISH} "Afinstaller SPF Framework fra din computer."
LangString IDS_UNCONF_SUBTITLE ${LANG_FINNISH} "Poista SPF Framework tietokoneeltasi."

LangString IDS_UNCONF_TEXT ${LANG_ENGLISH} "The following files will be removed:"
LangString IDS_UNCONF_TEXT ${LANG_UKRAINIAN} "Будуть видалені такі файли:"
LangString IDS_UNCONF_TEXT ${LANG_GERMAN} "Die folgenden Dateien werden entfernt:"
LangString IDS_UNCONF_TEXT ${LANG_FRENCH} "Les fichiers suivants seront supprimés :"
LangString IDS_UNCONF_TEXT ${LANG_SPANISH} "Se eliminarán los siguientes archivos:"
LangString IDS_UNCONF_TEXT ${LANG_POLISH} "Poniższe pliki zostaną usunięte:"
LangString IDS_UNCONF_TEXT ${LANG_RUSSIAN} "Будут удалены следующие файлы:"
LangString IDS_UNCONF_TEXT ${LANG_PORTUGUESEBR} "Os seguintes arquivos serão removidos:"
LangString IDS_UNCONF_TEXT ${LANG_PORTUGUESE} "Os seguintes ficheiros serão removidos:"
LangString IDS_UNCONF_TEXT ${LANG_ITALIAN} "I seguenti file verranno rimossi:"
LangString IDS_UNCONF_TEXT ${LANG_DUTCH} "De volgende bestanden worden verwijderd:"
LangString IDS_UNCONF_TEXT ${LANG_TURKISH} "Aşağıdaki dosyalar silinecek:"
LangString IDS_UNCONF_TEXT ${LANG_CZECH} "Následující soubory budou odstraněny:"
LangString IDS_UNCONF_TEXT ${LANG_HUNGARIAN} "A következő fájlok eltávolításra kerülnek:"
LangString IDS_UNCONF_TEXT ${LANG_SIMPCHINESE} "将删除以下文件："
LangString IDS_UNCONF_TEXT ${LANG_JAPANESE} "以下のファイルが削除されます："
LangString IDS_UNCONF_TEXT ${LANG_KOREAN} "다음 파일이 삭제됩니다:"
LangString IDS_UNCONF_TEXT ${LANG_NORWEGIAN} "Følgende filer vil bli fjernet:"
LangString IDS_UNCONF_TEXT ${LANG_DANISH} "Følgende filer vil blive fjernet:"
LangString IDS_UNCONF_TEXT ${LANG_FINNISH} "Seuraavat tiedostot poistetaan:"

LangString IDS_UNCONF_CHK1 ${LANG_ENGLISH} "Remove all SPF plugins"
LangString IDS_UNCONF_CHK1 ${LANG_UKRAINIAN} "Видалити всі SPF плагіни"
LangString IDS_UNCONF_CHK1 ${LANG_GERMAN} "Alle SPF-Plugins entfernen"
LangString IDS_UNCONF_CHK1 ${LANG_FRENCH} "Supprimer tous les plugins SPF"
LangString IDS_UNCONF_CHK1 ${LANG_SPANISH} "Eliminar todos los plugins SPF"
LangString IDS_UNCONF_CHK1 ${LANG_POLISH} "Usuń wszystkie wtyczki SPF"
LangString IDS_UNCONF_CHK1 ${LANG_RUSSIAN} "Удалить все SPF-плагины"
LangString IDS_UNCONF_CHK1 ${LANG_PORTUGUESEBR} "Remover todos os plugins SPF"
LangString IDS_UNCONF_CHK1 ${LANG_PORTUGUESE} "Remover todos os plugins SPF"
LangString IDS_UNCONF_CHK1 ${LANG_ITALIAN} "Rimuovi tutte le plugin SPF"
LangString IDS_UNCONF_CHK1 ${LANG_DUTCH} "Alle SPF-plugins verwijderen"
LangString IDS_UNCONF_CHK1 ${LANG_TURKISH} "Tüm SPF eklentilerini kaldır"
LangString IDS_UNCONF_CHK1 ${LANG_CZECH} "Odstranit všechny SPF pluginy"
LangString IDS_UNCONF_CHK1 ${LANG_HUNGARIAN} "Összes SPF plugin eltávolítása"
LangString IDS_UNCONF_CHK1 ${LANG_SIMPCHINESE} "删除所有 SPF 插件"
LangString IDS_UNCONF_CHK1 ${LANG_JAPANESE} "すべての SPF プラグインを削除"
LangString IDS_UNCONF_CHK1 ${LANG_KOREAN} "모든 SPF 플러그인 삭제"
LangString IDS_UNCONF_CHK1 ${LANG_NORWEGIAN} "Fjern alle SPF-tillegg"
LangString IDS_UNCONF_CHK1 ${LANG_DANISH} "Fjern alle SPF-plugins"
LangString IDS_UNCONF_CHK1 ${LANG_FINNISH} "Poista kaikki SPF-lisäosat"

LangString IDS_UNCONF_CHK2 ${LANG_ENGLISH} "Remove settings"
LangString IDS_UNCONF_CHK2 ${LANG_UKRAINIAN} "Видалити налаштування"
LangString IDS_UNCONF_CHK2 ${LANG_GERMAN} "Einstellungen entfernen"
LangString IDS_UNCONF_CHK2 ${LANG_FRENCH} "Supprimer les paramètres"
LangString IDS_UNCONF_CHK2 ${LANG_SPANISH} "Eliminar la configuración"
LangString IDS_UNCONF_CHK2 ${LANG_POLISH} "Usuń ustawienia"
LangString IDS_UNCONF_CHK2 ${LANG_RUSSIAN} "Удалить настройки"
LangString IDS_UNCONF_CHK2 ${LANG_PORTUGUESEBR} "Remover as configurações"
LangString IDS_UNCONF_CHK2 ${LANG_PORTUGUESE} "Remover as definições"
LangString IDS_UNCONF_CHK2 ${LANG_ITALIAN} "Rimuovi le impostazioni"
LangString IDS_UNCONF_CHK2 ${LANG_DUTCH} "Instellingen verwijderen"
LangString IDS_UNCONF_CHK2 ${LANG_TURKISH} "Ayarları kaldır"
LangString IDS_UNCONF_CHK2 ${LANG_CZECH} "Odstranit nastavení"
LangString IDS_UNCONF_CHK2 ${LANG_HUNGARIAN} "Beállítások eltávolítása"
LangString IDS_UNCONF_CHK2 ${LANG_SIMPCHINESE} "删除设置"
LangString IDS_UNCONF_CHK2 ${LANG_JAPANESE} "設定を削除"
LangString IDS_UNCONF_CHK2 ${LANG_KOREAN} "설정 삭제"
LangString IDS_UNCONF_CHK2 ${LANG_NORWEGIAN} "Fjern innstillinger"
LangString IDS_UNCONF_CHK2 ${LANG_DANISH} "Fjern indstillinger"
LangString IDS_UNCONF_CHK2 ${LANG_FINNISH} "Poista asetukset"

LangString IDS_UNCONF_WARN1 ${LANG_ENGLISH} "Warning: this will completely delete the folder with all installed plugins:"
LangString IDS_UNCONF_WARN1 ${LANG_UKRAINIAN} "Будьте уважні: це видалить повністю папку з усіма встановленими плагінами:"
LangString IDS_UNCONF_WARN1 ${LANG_GERMAN} "Achtung: Dies wird den Ordner mit allen installierten Plugins vollständig löschen:"
LangString IDS_UNCONF_WARN1 ${LANG_FRENCH} "Attention : cela supprimera complètement le dossier avec tous les plugins installés :"
LangString IDS_UNCONF_WARN1 ${LANG_SPANISH} "Atención: esto eliminará por completo la carpeta con todos los plugins instalados:"
LangString IDS_UNCONF_WARN1 ${LANG_POLISH} "Uwaga: to całkowicie usunie folder ze wszystkimi zainstalowanymi wtyczkami:"
LangString IDS_UNCONF_WARN1 ${LANG_RUSSIAN} "Внимание: это полностью удалит папку со всеми установленными плагинами:"
LangString IDS_UNCONF_WARN1 ${LANG_PORTUGUESEBR} "Atenção: isso excluirá completamente a pasta com todos os plugins instalados:"
LangString IDS_UNCONF_WARN1 ${LANG_PORTUGUESE} "Atenção: isto apagará completamente a pasta com todos os plugins instalados:"
LangString IDS_UNCONF_WARN1 ${LANG_ITALIAN} "Attenzione: ciò eliminerà completamente la cartella con tutte le plugin installate:"
LangString IDS_UNCONF_WARN1 ${LANG_DUTCH} "Opzicht: dit verwijdert de map met alle geïnstalleerde plugins volledig:"
LangString IDS_UNCONF_WARN1 ${LANG_TURKISH} "Dikkat: bu, kurulu tüm eklentilerin bulunduğu klasörü tamamen silecek:"
LangString IDS_UNCONF_WARN1 ${LANG_CZECH} "Varování: toto zcela odstraní složku se všemi nainstalovanými pluginy:"
LangString IDS_UNCONF_WARN1 ${LANG_HUNGARIAN} "Figyelem: ez teljesen törölni fogja a mappát az összes telepített pluginnel:"
LangString IDS_UNCONF_WARN1 ${LANG_SIMPCHINESE} "警告：这将完全删除包含所有已安装插件的文件夹："
LangString IDS_UNCONF_WARN1 ${LANG_JAPANESE} "警告：これはインストール済みプラグインすべてのフォルダーを完全に削除します："
LangString IDS_UNCONF_WARN1 ${LANG_KOREAN} "경고: 이렇게 하면 설치된 모든 플러그인이 있는 폴더가 완전히 삭제됩니다:"
LangString IDS_UNCONF_WARN1 ${LANG_NORWEGIAN} "Advarsel: dette vil fullstendig slette mappen med alle installerte tillegg:"
LangString IDS_UNCONF_WARN1 ${LANG_DANISH} "Advarsel: dette vil fuldstændigt slette mappen med alle installerede plugins:"
LangString IDS_UNCONF_WARN1 ${LANG_FINNISH} "Varoitus: tämä poistaa kokonaan kansion, jossa on kaikki asennetut lisäosat:"

LangString IDS_UNCONF_WARN2 ${LANG_ENGLISH} "Warning: this will completely delete the folder with all settings, localization and logs:"
LangString IDS_UNCONF_WARN2 ${LANG_UKRAINIAN} "Увага: це видалить повністю папку з усіма налаштуваннями, локалізацією і логами:"
LangString IDS_UNCONF_WARN2 ${LANG_GERMAN} "Achtung: Dies wird den Ordner mit allen Einstellungen, der Lokalisierung und den Protokollen vollständig löschen:"
LangString IDS_UNCONF_WARN2 ${LANG_FRENCH} "Attention : cela supprimera complètement le dossier avec tous les paramètres, la localisation et les journaux :"
LangString IDS_UNCONF_WARN2 ${LANG_SPANISH} "Atención: esto eliminará por completo la carpeta con toda la configuración, la localización y los registros:"
LangString IDS_UNCONF_WARN2 ${LANG_POLISH} "Uwaga: to całkowicie usunie folder ze wszystkimi ustawieniami, lokalizacją i logami:"
LangString IDS_UNCONF_WARN2 ${LANG_RUSSIAN} "Внимание: это полностью удалит папку со всеми настройками, локализацией и логами:"
LangString IDS_UNCONF_WARN2 ${LANG_PORTUGUESEBR} "Atenção: isso excluirá completamente a pasta com todas as configurações, localização e logs:"
LangString IDS_UNCONF_WARN2 ${LANG_PORTUGUESE} "Atenção: isto apagará completamente a pasta com todas as definições, localização e registos:"
LangString IDS_UNCONF_WARN2 ${LANG_ITALIAN} "Attenzione: ciò eliminerà completamente la cartella con tutte le impostazioni, la localizzazione e i log:"
LangString IDS_UNCONF_WARN2 ${LANG_DUTCH} "Opzicht: dit verwijdert de map met alle instellingen, lokalisatie en logs volledig:"
LangString IDS_UNCONF_WARN2 ${LANG_TURKISH} "Dikkat: bu, tüm ayarların, yerelleştirmenin ve günlüklerin bulunduğu klasörü tamamen silecek:"
LangString IDS_UNCONF_WARN2 ${LANG_CZECH} "Varování: toto zcela odstraní složku se všemi nastaveními, lokalizací a logy:"
LangString IDS_UNCONF_WARN2 ${LANG_HUNGARIAN} "Figyelem: ez teljesen törölni fogja a mappát az összes beállítással, helyi adattal és naplóval:"
LangString IDS_UNCONF_WARN2 ${LANG_SIMPCHINESE} "警告：这将完全删除包含所有设置、本地化和日志的文件夹："
LangString IDS_UNCONF_WARN2 ${LANG_JAPANESE} "警告：これはすべての設定、ローカライズ、ログのフォルダーを完全に削除します："
LangString IDS_UNCONF_WARN2 ${LANG_KOREAN} "경고: 이렇게 하면 모든 설정, 현지화 및 로그가 있는 폴더가 완전히 삭제됩니다:"
LangString IDS_UNCONF_WARN2 ${LANG_NORWEGIAN} "Advarsel: dette vil fullstendig slette mappen med alle innstillinger, lokalisering og logger:"
LangString IDS_UNCONF_WARN2 ${LANG_DANISH} "Advarsel: dette vil fuldstændigt slette mappen med alle indstillinger, lokalisering og logfiler:"
LangString IDS_UNCONF_WARN2 ${LANG_FINNISH} "Varoitus: tämä poistaa kokonaan kansion, jossa on kaikki asetukset, lokalisoinnit ja lokit:"

LangString IDS_WELCOME_TEXT ${LANG_ENGLISH} "SPF Framework will be installed on your computer.$\r$\n$\r$\nSPF Framework is a framework for American Truck Simulator and Euro Truck Simulator 2 that extends game functionality and provides an API for plugins."
LangString IDS_WELCOME_TEXT ${LANG_UKRAINIAN} "SPF Framework буде встановлено на ваш комп'ютер.$\r$\n$\r$\nSPF Framework — це фреймворк для American Truck Simulator і Euro Truck Simulator 2, який розширює функціональність гри та надає API для плагінів."
LangString IDS_WELCOME_TEXT ${LANG_GERMAN} "SPF Framework wird auf Ihrem Computer installiert.$\r$\n$\r$\nSPF Framework ist ein Framework für American Truck Simulator und Euro Truck Simulator 2, das die Spielfunktionen erweitert und eine API für Plugins bereitstellt."
LangString IDS_WELCOME_TEXT ${LANG_FRENCH} "SPF Framework sera installé sur votre ordinateur.$\r$\n$\r$\nSPF Framework est un framework pour American Truck Simulator et Euro Truck Simulator 2 qui étend les fonctionnalités du jeu et fournit une API pour les plugins."
LangString IDS_WELCOME_TEXT ${LANG_SPANISH} "SPF Framework se instalará en su equipo.$\r$\n$\r$\nSPF Framework es un framework para American Truck Simulator y Euro Truck Simulator 2 que amplía las funciones del juego y proporciona una API para plugins."
LangString IDS_WELCOME_TEXT ${LANG_POLISH} "SPF Framework zostanie zainstalowany na Twoim komputerze.$\r$\n$\r$\nSPF Framework to framework dla American Truck Simulator i Euro Truck Simulator 2, który rozszerza funkcje gry i udostępnia API dla wtyczek."
LangString IDS_WELCOME_TEXT ${LANG_RUSSIAN} "SPF Framework будет установлен на ваш компьютер.$\r$\n$\r$\nSPF Framework — это фреймворк для American Truck Simulator и Euro Truck Simulator 2, расширяющий функции игры и предоставляющий API для плагинов."
LangString IDS_WELCOME_TEXT ${LANG_PORTUGUESEBR} "SPF Framework será instalado em seu computador.$\r$\n$\r$\nSPF Framework é um framework para American Truck Simulator e Euro Truck Simulator 2 que expande as funções do jogo e fornece uma API para plugins."
LangString IDS_WELCOME_TEXT ${LANG_PORTUGUESE} "SPF Framework será instalado no seu computador.$\r$\n$\r$\nSPF Framework é uma plataforma para American Truck Simulator e Euro Truck Simulator 2 que expande as funções do jogo e fornece uma API para plugins."
LangString IDS_WELCOME_TEXT ${LANG_ITALIAN} "SPF Framework verrà installato sul tuo computer.$\r$\n$\r$\nSPF Framework è un framework per American Truck Simulator e Euro Truck Simulator 2 che estende le funzionalità di gioco e fornisce un'API per le plugin."
LangString IDS_WELCOME_TEXT ${LANG_DUTCH} "SPF Framework wordt op uw computer geïnstalleerd.$\r$\n$\r$\nSPF Framework is een framework voor American Truck Simulator en Euro Truck Simulator 2 dat de functies van het spel uitbreidt en een API biedt voor plugins."
LangString IDS_WELCOME_TEXT ${LANG_TURKISH} "SPF Framework bilgisayarınıza yüklenecek.$\r$\n$\r$\nSPF Framework, American Truck Simulator ve Euro Truck Simulator 2 için oyun işlevlerini genişleten ve eklentiler için bir API sağlayan bir çerçevedir."
LangString IDS_WELCOME_TEXT ${LANG_CZECH} "SPF Framework bude nainstalován do vašeho počítače.$\r$\n$\r$\nSPF Framework je framework pro American Truck Simulator a Euro Truck Simulator 2, který rozšiřuje funkce hry a poskytuje API pro pluginy."
LangString IDS_WELCOME_TEXT ${LANG_HUNGARIAN} "A SPF Framework telepítésre kerül a számítógépre.$\r$\n$\r$\nA SPF Framework egy keretrendszer az American Truck Simulator és a Euro Truck Simulator 2 játékokhoz, amely bővíti a játék funkcióit és API-t biztosít a bővítményekhez."
LangString IDS_WELCOME_TEXT ${LANG_SIMPCHINESE} "SPF Framework 将安装到您的计算机上。$\r$\n$\r$\nSPF Framework 是适用于 American Truck Simulator 和 Euro Truck Simulator 2 的框架，可扩展游戏功能并为插件提供 API。"
LangString IDS_WELCOME_TEXT ${LANG_JAPANESE} "SPF Framework がお使いのコンピューターにインストールされます。$\r$\n$\r$\nSPF Framework は American Truck Simulator と Euro Truck Simulator 2 向けのフレームワークで、ゲームの機能を拡張し、プラグイン用の API を提供します。"
LangString IDS_WELCOME_TEXT ${LANG_KOREAN} "SPF Framework가 컴퓨터에 설치됩니다.$\r$\n$\r$\nSPF Framework는 American Truck Simulator 및 Euro Truck Simulator 2용 프레임워크로, 게임 기능을 확장하고 플러그인을 위한 API를 제공합니다."
LangString IDS_WELCOME_TEXT ${LANG_NORWEGIAN} "SPF Framework vil bli installert på datamaskinen.$\r$\n$\r$\nSPF Framework er et rammeverk for American Truck Simulator og Euro Truck Simulator 2 som utvider spillfunksjonene og tilbyr et API for tillegg."
LangString IDS_WELCOME_TEXT ${LANG_DANISH} "SPF Framework installeres på din computer.$\r$\n$\r$\nSPF Framework er et framework til American Truck Simulator og Euro Truck Simulator 2, der udvider spilfunktionerne og tilbyder et API til plugins."
LangString IDS_WELCOME_TEXT ${LANG_FINNISH} "SPF Framework asennetaan tietokoneelle.$\r$\n$\r$\nSPF Framework on kehyspohja American Truck Simulatorille ja Euro Truck Simulator 2:lle, laajentaa pelin toimintoja ja tarjoaa API:n lisäosille."

LangString IDS_DOWNGRADE ${LANG_ENGLISH} "SPF Framework version $0 is already installed.$\r$\nDo you really want to downgrade to version ${APP_VERSION}?"
LangString IDS_DOWNGRADE ${LANG_UKRAINIAN} "SPF Framework версії $0 вже встановлено.$\r$\nДійсно хочете понизити версію до ${APP_VERSION}?"
LangString IDS_DOWNGRADE ${LANG_GERMAN} "SPF Framework Version $0 ist bereits installiert.$\r$\nMöchten Sie wirklich auf Version ${APP_VERSION} herabstufen?"
LangString IDS_DOWNGRADE ${LANG_FRENCH} "La version $0 de SPF Framework est déjà installée.$\r$\nVoulez-vous vraiment rétrograder vers la version ${APP_VERSION} ?"
LangString IDS_DOWNGRADE ${LANG_SPANISH} "La versión $0 de SPF Framework ya está instalada.$\r$\n¿Realmente desea degradar a la versión ${APP_VERSION}?"
LangString IDS_DOWNGRADE ${LANG_POLISH} "Wersja $0 SPF Framework jest już zainstalowana.$\r$\nCzy na pewno chcesz obniżyć wersję do ${APP_VERSION}?"
LangString IDS_DOWNGRADE ${LANG_RUSSIAN} "SPF Framework версии $0 уже установлен.$\r$\nВы действительно хотите понизить версию до ${APP_VERSION}?"
LangString IDS_DOWNGRADE ${LANG_PORTUGUESEBR} "A versão $0 do SPF Framework já está instalada.$\r$\nDeseja mesmo fazer downgrade para a versão ${APP_VERSION}?"
LangString IDS_DOWNGRADE ${LANG_PORTUGUESE} "A versão $0 do SPF Framework já está instalada.$\r$\nDeseja mesmo fazer downgrade para a versão ${APP_VERSION}?"
LangString IDS_DOWNGRADE ${LANG_ITALIAN} "La versione $0 di SPF Framework è già installata.$\r$\nVuoi davvero degradare alla versione ${APP_VERSION}?"
LangString IDS_DOWNGRADE ${LANG_DUTCH} "SPF Framework versie $0 is al geïnstalleerd.$\r$\nWilt u echt downgraden naar versie ${APP_VERSION}?"
LangString IDS_DOWNGRADE ${LANG_TURKISH} "SPF Framework sürüm $0 zaten yüklü.$\r$\nGerçekten ${APP_VERSION} sürümüne düşürmek istiyor musunuz?"
LangString IDS_DOWNGRADE ${LANG_CZECH} "SPF Framework verze $0 je již nainstalována.$\r$\nOpravdu chcete přejít na verzi ${APP_VERSION}?"
LangString IDS_DOWNGRADE ${LANG_HUNGARIAN} "A SPF Framework $0 verziója már telepítve van.$\r$\nValóban vissza szeretné telepíteni a(z) ${APP_VERSION} verziót?"
LangString IDS_DOWNGRADE ${LANG_SIMPCHINESE} "已安装 SPF Framework 版本 $0。$\r$\n确实要降级到版本 ${APP_VERSION} 吗？"
LangString IDS_DOWNGRADE ${LANG_JAPANESE} "SPF Framework バージョン $0 は既にインストールされています。$\r$\nバージョン ${APP_VERSION} にダウングレードしますか？"
LangString IDS_DOWNGRADE ${LANG_KOREAN} "SPF Framework 버전 $0이(가) 이미 설치되어 있습니다.$\r$\n버전 ${APP_VERSION}(으)로 다운그레이드하시겠습니까?"
LangString IDS_DOWNGRADE ${LANG_NORWEGIAN} "SPF Framework versjon $0 er allerede installert.$\r$\nVil du virkelig nedgradere til versjon ${APP_VERSION}?"
LangString IDS_DOWNGRADE ${LANG_DANISH} "SPF Framework version $0 er allerede installeret.$\r$\nVil du virkelig nedgradere til version ${APP_VERSION}?"
LangString IDS_DOWNGRADE ${LANG_FINNISH} "SPF Framework -versio $0 on jo asennettu.$\r$\nHaluatko todesti palauttaa version ${APP_VERSION}?"

; ---------------------------------------------------------------------------
; Custom messages (English only here; NSIS uses built-in language files for
; the standard UI, and these labels mirror the Inno CustomMessages).
; ---------------------------------------------------------------------------

; ---------------------------------------------------------------------------
; Sections
; ---------------------------------------------------------------------------
Section "!SPF Framework" SecCore
  SectionIn RO
  SetOutPath "$INSTDIR"
  File "icon.ico"
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKCU "${REG_KEY}" "InstallDir" "$INSTDIR"

  ; Programs and Features entry (per-user, visible in Apps & Features)
  WriteRegStr HKCU "${UNINSTALL_KEY}" "DisplayName" "${APP_NAME}"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "DisplayVersion" "${APP_VERSION}"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "Publisher" "${APP_PUBLISHER}"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "DisplayIcon" "$INSTDIR\icon.ico"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "UninstallString" "$INSTDIR\uninst.exe\"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "URLInfoAbout" "${APP_URL}"
  WriteRegDWORD HKCU "${UNINSTALL_KEY}" "NoModify" 1
  WriteRegDWORD HKCU "${UNINSTALL_KEY}" "NoRepair" 1
  ${GetSize} "$INSTDIR" "/S=1K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKCU "${UNINSTALL_KEY}" "EstimatedSize" "$0"
SectionEnd

SectionGroup /e "$(IDS_SEC_GAMES)" SecGames
  Section "American Truck Simulator" SecATS
    SetOverwrite on
    ; DLL
    SetOutPath "$ATSPath\bin\win_x64\plugins"
    File "dist\spf-framework.dll"
    ; Assets (recursive)
    SetOutPath "$ATSPath\bin\win_x64\plugins\spfAssets"
    File /r "dist\spfAssets\*"
    ; Create spfPlugins dir (never removed on uninstall)
    CreateDirectory "$ATSPath\bin\win_x64\plugins\spfPlugins"
    ; Registry
    WriteRegStr HKCU "${REG_KEY}" "DllPathATS" "$ATSPath\bin\win_x64\plugins\spf-framework.dll"
  SectionEnd

  Section "Euro Truck Simulator 2" SecETS2
    SetOverwrite on
    SetOutPath "$ETS2Path\bin\win_x64\plugins"
    File "dist\spf-framework.dll"
    SetOutPath "$ETS2Path\bin\win_x64\plugins\spfAssets"
    File /r "dist\spfAssets\*"
    CreateDirectory "$ETS2Path\bin\win_x64\plugins\spfPlugins"
    WriteRegStr HKCU "${REG_KEY}" "DllPathETS2" "$ETS2Path\bin\win_x64\plugins\spf-framework.dll"
  SectionEnd
SectionGroupEnd

Section "$(IDS_SEC_SHORTCUT)" SecShortcut
  IfFileExists "$ATSPath\bin\win_x64\plugins" 0 +2
    CreateShortCut "$DESKTOP\ATS Plugin Folder.lnk" "$ATSPath\bin\win_x64\plugins"
  IfFileExists "$ETS2Path\bin\win_x64\plugins" 0 +2
    CreateShortCut "$DESKTOP\ETS2 Plugin Folder.lnk" "$ETS2Path\bin\win_x64\plugins"
SectionEnd

; ---------------------------------------------------------------------------
; Uninstaller
; ---------------------------------------------------------------------------
Function un.onInit
  !insertmacro MUI_UNGETLANGUAGE
  ; Capture recorded DLL paths before we clear the registry
  ReadRegStr $DLLPathATS HKCU "${REG_KEY}" "DllPathATS"
  ReadRegStr $DLLPathETS2 HKCU "${REG_KEY}" "DllPathETS2"
  StrCpy $UnDoPlugins 0
  StrCpy $UnDoSettings 0
FunctionEnd

Function un.ConfirmPage
  nsDialogs::Create 1018
  Pop $0
  ${If} $0 == error
    Abort
  ${EndIf}

  !insertmacro MUI_HEADER_TEXT "$(IDS_UNCONF_TITLE)" "$(IDS_UNCONF_SUBTITLE)"

  StrCpy $0 "$(IDS_UNCONF_TEXT)$\r$\n"
  ${If} $DLLPathATS != ""
    StrCpy $0 "$0$DLLPathATS$\r$\n"
  ${EndIf}
  ${If} $DLLPathETS2 != ""
    StrCpy $0 "$0$DLLPathETS2$\r$\n"
  ${EndIf}
  ${NSD_CreateLabel} 0 0 100% 50u "$0"
  Pop $0

  ${NSD_CreateCheckbox} 0 60u 100% 12u "$(IDS_UNCONF_CHK1)"
  Pop $UnChkPlugins
  ${NSD_OnClick} $UnChkPlugins un.OnChkPlugins

  ${NSD_CreateCheckbox} 0 80u 100% 12u "$(IDS_UNCONF_CHK2)"
  Pop $UnChkSettings
  ${NSD_OnClick} $UnChkSettings un.OnChkSettings

  nsDialogs::Show
FunctionEnd

Function un.OnChkPlugins
  Pop $0
  ${NSD_GetState} $UnChkPlugins $0
  ${If} $0 == ${BST_CHECKED}
    StrCpy $UnDoPlugins 1
    StrCpy $1 "$(IDS_UNCONF_WARN1)$\r$\n"
    ${If} $DLLPathATS != ""
      ${GetParent} $DLLPathATS $2
      StrCpy $1 "$1$2\spfPlugins$\r$\n"
    ${EndIf}
    ${If} $DLLPathETS2 != ""
      ${GetParent} $DLLPathETS2 $2
      StrCpy $1 "$1$2\spfPlugins$\r$\n"
    ${EndIf}
    MessageBox MB_ICONEXCLAMATION|MB_OK "$1"
  ${Else}
    StrCpy $UnDoPlugins 0
  ${EndIf}
FunctionEnd

Function un.OnChkSettings
  Pop $0
  ${NSD_GetState} $UnChkSettings $0
  ${If} $0 == ${BST_CHECKED}
    StrCpy $UnDoSettings 1
    StrCpy $1 "$(IDS_UNCONF_WARN2)$\r$\n"
    ${If} $DLLPathATS != ""
      ${GetParent} $DLLPathATS $2
      StrCpy $1 "$1$2\spfAssets$\r$\n"
    ${EndIf}
    ${If} $DLLPathETS2 != ""
      ${GetParent} $DLLPathETS2 $2
      StrCpy $1 "$1$2\spfAssets$\r$\n"
    ${EndIf}
    MessageBox MB_ICONEXCLAMATION|MB_OK "$1"
  ${Else}
    StrCpy $UnDoSettings 0
  ${EndIf}
FunctionEnd

Section "Uninstall"
  ; Always: DLL + spfAssets\localization (copied by installer).
  ; Checkbox "plugins": whole spfPlugins. Checkbox "settings": whole spfAssets.
  ${If} $DLLPathATS != ""
    Delete "$DLLPathATS"
    ${GetParent} $DLLPathATS $0
    ${If} $UnDoSettings == 1
      RMDir /r "$0\spfAssets"
    ${Else}
      RMDir /r "$0\spfAssets\localization"
    ${EndIf}
    ${If} $UnDoPlugins == 1
      RMDir /r "$0\spfPlugins"
    ${EndIf}
  ${EndIf}
  ${If} $DLLPathETS2 != ""
    Delete "$DLLPathETS2"
    ${GetParent} $DLLPathETS2 $0
    ${If} $UnDoSettings == 1
      RMDir /r "$0\spfAssets"
    ${Else}
      RMDir /r "$0\spfAssets\localization"
    ${EndIf}
    ${If} $UnDoPlugins == 1
      RMDir /r "$0\spfPlugins"
    ${EndIf}
  ${EndIf}

  Delete "$DESKTOP\ATS Plugin Folder.lnk"
  Delete "$DESKTOP\ETS2 Plugin Folder.lnk"

  DeleteRegValue HKCU "${REG_KEY}" "Version"
  DeleteRegValue HKCU "${REG_KEY}" "DllPathATS"
  DeleteRegValue HKCU "${REG_KEY}" "DllPathETS2"
  DeleteRegKey HKCU "${REG_KEY}"

  Delete "$INSTDIR\uninst.exe"
  Delete "$INSTDIR\icon.ico"
  DeleteRegKey HKCU "${UNINSTALL_KEY}"
  DeleteRegKey HKCU "Software\${APP_NAME}"
  RMDir "$INSTDIR"
SectionEnd

; ---------------------------------------------------------------------------
; Get game path from Steam library files (user-level, no admin, no registry)
; Steam stores its libraries in <steamroot>\steamapps\libraryfolders.vdf and
; each game's install dir in <library>\steamapps\appmanifest_<appid>.acf.
; in:  $0 = AppID ; out: $R0 = game dir ("" if not found)
; ---------------------------------------------------------------------------
Function GetGamePath
  Push $R0
  Push $0
  Push $1
  Push $2
  Push $3
  Push $4
  Push $5
  StrCpy $R0 ""
  StrCpy $5 $0                      ; $5 = AppID (preserved)

  ; 1. Locate the Steam root (kept in $1, NOT in $R0 which is our result)
  Call GetSteamRoot
  StrCpy $1 $R0                  ; $1 = steam root
  StrCpy $R0 ""                  ; clear result register (GetSteamRoot wrote it)
  StrCmp $1 "" getpath_done 0

  ; 2. Open libraryfolders.vdf and scan every "path" entry.
  ;    Each library path is stored in $4. For each, look for the app manifest
  ;    (in both <lib>\ and <lib>\steamapps\ layouts) and read "installdir".
  ;    Try both steamapps\libraryfolders.vdf and config\libraryfolders.vdf.
  FileOpen $2 "$1\steamapps\libraryfolders.vdf" r
  IfErrors 0 getpath_loop_start
  FileOpen $2 "$1\config\libraryfolders.vdf" r
  IfErrors getpath_done
  getpath_loop_start:
  getpath_loop:
    FileRead $2 $3
    IfErrors getpath_close
    StrCpy $0 $3                  ; keep the raw line in $0

    ; --- extract first quoted token of this line ---
    !insertmacro GET_STRING_TOKEN $0 1
    Pop $4
    StrCmp $4 "path" 0 getpath_loop

    ; --- this is a "path" entry: value = library root ---
    !insertmacro GET_STRING_TOKEN $0 2
    Pop $4

    ; the path in the VDF uses escaped backslashes and may or may not end
    ; with \steamapps depending on the Steam version
    ${WordReplace} $4 "\\" "\" "+*" $4

    ; check manifest in <lib> first, then <lib>\steamapps
    IfFileExists "$4\appmanifest_$5.acf" 0 try_steamapps
      Call GetManifestInstalldir
      Goto getpath_close
    try_steamapps:
    IfFileExists "$4\steamapps\appmanifest_$5.acf" 0 getpath_loop
      StrCpy $4 "$4\steamapps"
      Call GetManifestInstalldir
      Goto getpath_close

  getpath_close:
    FileClose $2

  getpath_done:
  Pop $5
  Pop $4
  Pop $3
  Pop $2
  Pop $1
  Pop $0
  Exch $R0
FunctionEnd

; ---------------------------------------------------------------------------
; Read "installdir" from an appmanifest_<appid>.acf and build the game dir.
; in:  $4 = library dir (already has \steamapps if needed), $5 = AppID
; out: $R0 = "<lib>\common\<installdir>" ("" if not found)
; uses $1-$3 (preserved)
; ---------------------------------------------------------------------------
Function GetManifestInstalldir
  Push $1
  Push $2
  Push $3
  Push $6
  StrCpy $R0 ""
  FileOpen $6 "$4\appmanifest_$5.acf" r
  IfErrors gmi_done
  gmi_loop:
    FileRead $6 $3
    IfErrors gmi_close
    StrCpy $1 $3
    !insertmacro GET_STRING_TOKEN $1 1
    Pop $2
    StrCmp $2 "installdir" 0 gmi_loop
    !insertmacro GET_STRING_TOKEN $1 2
    Pop $2
    StrCpy $R0 "$4\common\$2"
    Goto gmi_close
  gmi_close:
    FileClose $6
  gmi_done:
  Pop $6
  Pop $3
  Pop $2
  Pop $1
FunctionEnd

; ---------------------------------------------------------------------------
; Locate the Steam install directory.
; out: $R0 = steam root ("" if not found)
; ---------------------------------------------------------------------------
Function GetSteamRoot
  StrCpy $R0 ""

  ; 1. HKCU — user-level install, no admin needed
  ReadRegStr $R0 HKCU "Software\Valve\Steam" "Path"
  IfErrors 0 check_steamexe
  ReadRegStr $R0 HKCU "Software\Valve\Steam" "SteamPath"
  IfErrors 0 check_steamexe

  ; 2. HKLM 64-bit view
  SetRegView 64
  ReadRegStr $R0 HKLM "Software\Valve\Steam" "InstallPath"
  IfErrors 0 check_steamexe

  ; 3. HKLM 32-bit view
  SetRegView 32
  ReadRegStr $R0 HKLM "Software\Valve\Steam" "InstallPath"
  IfErrors 0 check_steamexe

  ; 4. Common default paths
  StrCpy $R0 "$PROGRAMFILES32\Steam"
  IfFileExists "$R0\steam.exe" check_steamexe
  StrCpy $R0 "$PROGRAMFILES\Steam"
  IfFileExists "$R0\steam.exe" check_steamexe
  StrCpy $R0 "$COMMONFILES\Steam"
  IfFileExists "$R0\steam.exe" check_steamexe
  StrCpy $R0 ""
  Return

  check_steamexe:
  IfFileExists "$R0\steam.exe" 0 steam_root_bad
    ; modern Steam writes SteamPath with forward slashes (c:/...); NSIS FileOpen
    ; rejects mixed-slash paths — normalize to backslashes for all path building
    ${WordReplace} $R0 "/" "\" "+*" $R0
    Return
  steam_root_bad:
  StrCpy $R0 ""
FunctionEnd

; ---------------------------------------------------------------------------
; Helpers for section-selected checks
; ---------------------------------------------------------------------------
Function IsSecATS
  SectionGetFlags ${SecATS} $0
  IntOp $0 $0 & ${SF_SELECTED}
  StrCmp $0 ${SF_SELECTED} 0 isec_ats_zero
    Push 1
    Return
  isec_ats_zero:
    Push 0
FunctionEnd

Function IsSecETS2
  SectionGetFlags ${SecETS2} $0
  IntOp $0 $0 & ${SF_SELECTED}
  StrCmp $0 ${SF_SELECTED} 0 isec_ets2_zero
    Push 1
    Return
  isec_ets2_zero:
    Push 0
FunctionEnd

Function PageConfirmPaths
  nsDialogs::Create 1018
  Pop $0
  ${If} $0 == error
    Abort
  ${EndIf}

  ${NSD_CreateLabel} 0 0 100% 12u "$(IDS_PATHS_TITLE)"
  Pop $0
  ${NSD_CreateLabel} 0 20u 100% 12u "$(IDS_PATHS_DESC)"
  Pop $0

  Call IsSecATS
  Pop $0
  ${If} $0 = 1
    ${NSD_CreateLabel} 0 40u 100% 12u "$(IDS_ATS_PATH)"
    Pop $0
    ${NSD_CreateDirRequest} 0 55u 75% 12u "$ATSPath"
    Pop $ATSPathEdit
    ${NSD_CreateBrowseButton} 78% 55u 22% 12u "$(IDS_BROWSE)"
    Pop $0
    ${NSD_OnClick} $0 OnBrowseATSPath
  ${EndIf}

  Call IsSecETS2
  Pop $0
  ${If} $0 = 1
    ${NSD_CreateLabel} 0 80u 100% 12u "$(IDS_ETS2_PATH)"
    Pop $0
    ${NSD_CreateDirRequest} 0 95u 75% 12u "$ETS2Path"
    Pop $ETS2PathEdit
    ${NSD_CreateBrowseButton} 78% 95u 22% 12u "$(IDS_BROWSE)"
    Pop $0
    ${NSD_OnClick} $0 OnBrowseETS2Path
  ${EndIf}

  nsDialogs::Show
FunctionEnd

Function OnBrowseATSPath
  ${NSD_GetText} $ATSPathEdit $0
  nsDialogs::SelectFolderDialog "$(IDS_SELECT_ATS)" "$0"
  Pop $0
  ${If} $0 != error
    ${NSD_SetText} $ATSPathEdit "$0"
  ${EndIf}
FunctionEnd

Function OnBrowseETS2Path
  ${NSD_GetText} $ETS2PathEdit $0
  nsDialogs::SelectFolderDialog "$(IDS_SELECT_ETS2)" "$0"
  Pop $0
  ${If} $0 != error
    ${NSD_SetText} $ETS2PathEdit "$0"
  ${EndIf}
FunctionEnd

Function PageConfirmPathsLeave
  Call IsSecATS
  Pop $0
  ${If} $0 = 1
    ${NSD_GetText} $ATSPathEdit $ATSPath
    ${IfNot} ${FileExists} "$ATSPath\bin\win_x64"
      MessageBox MB_ICONEXCLAMATION|MB_OK "$(IDS_INVALID_ATS)"
      Abort
    ${EndIf}
  ${EndIf}

  Call IsSecETS2
  Pop $0
  ${If} $0 = 1
    ${NSD_GetText} $ETS2PathEdit $ETS2Path
    ${IfNot} ${FileExists} "$ETS2Path\bin\win_x64"
      MessageBox MB_ICONEXCLAMATION|MB_OK "$(IDS_INVALID_ETS2)"
      Abort
    ${EndIf}
  ${EndIf}
FunctionEnd

; ---------------------------------------------------------------------------
; Init: downgrade check + auto-detect game paths
; ---------------------------------------------------------------------------
Function .onInit
  !insertmacro MUI_LANGDLL_DISPLAY

  ; --- Registry unification: migrate legacy InstallDir into Software\SPF_Framework
  ; and drop the old installer-only key plus the Inno uninstall entry.
  ClearErrors
  ReadRegStr $0 HKCU "${REG_KEY}" "InstallDir"
  IfErrors 0 legacy_dir_done
    ClearErrors
    ReadRegStr $0 HKCU "Software\${APP_NAME}" "InstallDir"
    IfErrors legacy_dir_done
      WriteRegStr HKCU "${REG_KEY}" "InstallDir" $0
      StrCpy $INSTDIR $0
  legacy_dir_done:
  DeleteRegKey HKCU "Software\${APP_NAME}"
  DeleteRegKey HKCU "${LEGACY_UNINSTALL_KEY}"
  SetRegView 64
  DeleteRegKey HKLM "${LEGACY_UNINSTALL_KEY}"
  SetRegView 32
  DeleteRegKey HKLM "${LEGACY_UNINSTALL_KEY}"

  ; Downgrade check: installed newer than ${APP_VERSION} -> warn
  ClearErrors
  ReadRegStr $0 HKCU "${REG_KEY}" "Version"
  IfErrors version_check_done
  ${VersionCompare} "${APP_VERSION}" "$0" $1
  ${If} $1 == 2
    MessageBox MB_ICONEXCLAMATION|MB_YESNO "$(IDS_DOWNGRADE)" IDYES version_check_done
    Abort
  ${EndIf}
  version_check_done:

  ; Auto-detect ATS path
  StrCpy $0 "${STEAM_APPID_ATS}"
  Call GetGamePath
  Pop $ATSPath

  ; Auto-detect ETS2 path
  StrCpy $0 "${STEAM_APPID_ETS2}"
  Call GetGamePath
  Pop $ETS2Path

  IfErrors migrate_legacy done_migrate
  migrate_legacy:
    ReadRegStr $1 HKLM "${REG_KEY}" "Version"
    IfErrors done_migrate 0
      WriteRegStr HKCU "${REG_KEY}" "Version" $1
  done_migrate:
FunctionEnd

; ---------------------------------------------------------------------------
; Post-install: write version info
; ---------------------------------------------------------------------------
Function .onInstSuccess
  WriteRegStr HKCU "${REG_KEY}" "Version" "${APP_VERSION}"
FunctionEnd
