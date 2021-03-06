// SIMULATE
template<UINT32 lengthInBits> void STRINGOP::sMOVS
    (ADDRINT count, ADDRINT flagsValue, ADDRINT readAddress, ADDRINT writeAddress, ADDRINT insAddress) 
{
    // copie de "count" octets/mot/double mots de [ESI] vers [EDI] (idem MOV mem->mem)
    ADDRINT nbIterations = count * (lengthInBits >> 3); // nombre d'octets copi�s
    
    // direction Flag mis � 1 : les adresses vont d�croissantes, sinon ca sera l'inverse
    UINT32 isDecrease = (flagsValue >> DIRECTION_FLAG) & 1; 
    
    for (UINT32 counter = 0 ; counter < nbIterations; ++counter)
    {
        if (pTmgrGlobal->isMemoryTainted<8>(readAddress))	
        {
            _LOGTAINT(PIN_ThreadId(), insAddress, "MOVS" + decstr(lengthInBits) + " iterations = " + decstr(nbIterations) + " flagsValue = " + decstr(flagsValue));
            pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                X_ASSIGN,
                ObjectSource(pTmgrGlobal->getMemoryTaint<8>(readAddress))));
        }
        else pTmgrGlobal->unTaintMemory<8>(writeAddress);
            
        if (isDecrease)
        {
            --readAddress; --writeAddress;
        }
        else
        {
            ++readAddress; ++writeAddress;
        }
    }
} //sMOVS

template<UINT32 lengthInBits> void STRINGOP::sLODS
    (THREADID tid, ADDRINT count, ADDRINT flagsValue, ADDRINT readAddress, ADDRINT insAddress) 
{
    // d�placement de "count" octets/mot/dble mots de [ESI] vers AL/AX/EAX/RAX
    // en r�alit�, seule la derni�re it�ration nous interesse
    // ce sera un MOV_MR<lengthInBits> de lastAddress vers AL/AX/EAX/RAX
  
    // calcul de l'adresse point�e par ESI lors de la derni�re op�ration
    // DF a 1 => adresses d�croissantes, DF a 0 => adresses croissantes
    ADDRINT lastAddress = ((flagsValue >> DIRECTION_FLAG) & 1) 
        ? (readAddress - ((count-1) * (lengthInBits >> 3))) 
        : (readAddress + ((count-1) * (lengthInBits >> 3))); 
    
    // appel de la proc�dure MOVMR ad�quate selon le registre
    _LOGTAINT(tid, insAddress, "LODS (si marqu� : message movRM suivra)");
    DATAXFER::sMOV_MR<lengthInBits>(tid, lastAddress, RegisterACC<lengthInBits>::getReg() ,insAddress);
} // sLODS

template<UINT32 lengthInBits> void STRINGOP::sSTOS
    (THREADID tid, ADDRINT count, ADDRINT flagsValue, ADDRINT writeAddress, ADDRINT insAddress) 
{
    // d�placement de "count" octets/mot/dble mots de AL/AX/EAX/RAX -> [EDI] 
    
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    const UINT32 opSize = lengthInBits >> 3;

    // 1) recup�ration du marquage de chaque octet du registre
    // et stockage dans un tableau d'objets (ou nullPtr)
    std::array<TaintBytePtr, opSize> vTaintRegSrc;

    for (UINT32 regPart = 0 ; regPart < opSize ; ++regPart) 
    {
        #if TARGET_IA32
        vTaintRegSrc[regPart] = pTmgrTls->getRegisterPartTaint(regIndexEAX, regPart);
        #else
        vTaintRegSrc[regPart] = pTmgrTls->getRegisterPartTaint(regIndexRAX, regPart);
        #endif  
    }

    // si DF = 1 => adresses d�croissantes, sinon le contraire
    UINT32 isDecrease = (flagsValue >> DIRECTION_FLAG) & 1; 

    for (UINT32 iteration = 0 ; iteration < count ; ++iteration) // r�p�tition "count" fois
    {
        // marquage de chaque plage de 'lengthInBits' octets avec le registre source     
        for (auto it = vTaintRegSrc.begin(); it != vTaintRegSrc.end() ; ++it) 
        {
            if ((bool) *it)
            {
                pTmgrGlobal->updateMemoryTaint<8>(writeAddress, std::make_shared<TaintByte>(
                    X_ASSIGN,
                    ObjectSource(*it)));
            }
            else pTmgrGlobal->unTaintMemory<8>(writeAddress);

            // calcul de la prochaine adresse
            (isDecrease) ? --writeAddress : ++writeAddress;
        }
    }
} // sSTOS

template<UINT32 lengthInBits> void STRINGOP::sFirstRepScas(
    THREADID tid, BOOL isREPZ, ADDRINT regValue, ADDRINT insAddress) 
{ 
    // proc�dure appel�e lors de la premi�re it�ration (callback IF/THEN)
    // sert � stocker dans TaintManager les arguments (marquage, adresse)
    
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    StringOpInfo    *pStringOpInfo = static_cast<StringOpInfo*>(PIN_GetThreadData(g_tlsStringOpInfo, tid));

    ZeroMemory(pStringOpInfo, sizeof(StringOpInfo));

    // mise en cache de la valeur du pr�fixe (true = REPZ/E, false = REPNZ/E) et de l'adresse
    pStringOpInfo->isREPZ     = isREPZ; 
    pStringOpInfo->insAddress = insAddress;
    
    // si registre marqu�, stockage de l'objet, sinon mise � nullptr 
    // et stockage de la valeur
    if (pTmgrTls->isRegisterTainted<lengthInBits>(RegisterACC<lengthInBits>::getReg())) 
    {
        pStringOpInfo->isRegTainted = true;
        pStringOpInfo->tPtr = pTmgrTls->getRegisterTaint<lengthInBits>(
            RegisterACC<lengthInBits>::getReg(), regValue);
    }
    else 
    {
        pStringOpInfo->isRegTainted = false; 
        pStringOpInfo->regValue = regValue; // stockage de la valeur du registre
    }
} // storeTaintSCAS

template<UINT32 lengthInBits> void STRINGOP::sSCAS(THREADID tid, ADDRINT address) 
{
    // comparaison de AL/AX/EAX/RAX avec [EDI] 
    // + ajout d'une contrainte sur ZFLAG, indiquant une r�p�tition ou non
    // m�tohde identique � CMP_MR, reprise ici int�gralement afin d'ins�rer la contrainte sur le ZFLAG
    
    TaintManager_Thread *pTmgrTls  = getTmgrInTls(tid);
    StringOpInfo    *pStringOpInfo = static_cast<StringOpInfo*>(PIN_GetThreadData(g_tlsStringOpInfo, tid));
    
    bool isSrcDestTainted = pStringOpInfo->isRegTainted;
    bool isSrcTainted =		pTmgrGlobal->isMemoryTainted<lengthInBits>(address);

    if ( !(isSrcDestTainted || isSrcTainted))   pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, pStringOpInfo->insAddress, "SCAS");
        
        // calcul des sources pour le futur objet
        ObjectSource srcDest = (isSrcDestTainted) 
            ? ObjectSource(pStringOpInfo->tPtr) : ObjectSource(lengthInBits, pStringOpInfo->regValue);
        
        ObjectSource src = (isSrcTainted) 
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(address))
            : ObjectSource(lengthInBits, getMemoryValue<lengthInBits>(address));

        // marquage flags 
        BINARY::fTaintCMP<lengthInBits>(pTmgrTls, srcDest, src);

        /** TODO : RETRAVAILLER LA CONDITION DE LA CONTRAINTE **/
        /** EN EFFET LE PREDICAT NE PORTE QUE SUR ECX/RCX : RIEN N'INDIQUE **/
        /** LA POSITION SUR LE ZFLAG. SEULE SOLUTION POUR INSERER UNE CONTRAINTE : **/
        /** TESTER EN INSTRUMENTANT EN IPOINT_AFTER ET TESTER LES FLAGS **/
#if 0
        // d�claration de la contrainte
        // Le predicat est forc�ment vrai puisque la fonction d'analyse a �t� appel�e
        PREDICATE pred = pScasInfo->isREPZ ? PREDICATE_ZERO : PREDICATE_NOT_ZERO;
        g_pFormula->addConstraintJcc(pTmgrTls, pred, true, pStringOpInfo->insAddress);
#endif

    }
} // sSCAS

template<UINT32 lengthInBits> void STRINGOP::sCMPS
    (THREADID tid, UINT32 repCode, ADDRINT esiAddr, ADDRINT ediAddr, ADDRINT insAddress) 
{
    bool isSrcTainted =		pTmgrGlobal->isMemoryTainted<lengthInBits>(esiAddr);
    bool isSrcDestTainted = pTmgrGlobal->isMemoryTainted<lengthInBits>(ediAddr);	
    
    TaintManager_Thread *pTmgrTls = getTmgrInTls(tid);
    
    if ( !(isSrcDestTainted || isSrcTainted)) pTmgrTls->unTaintAllFlags();
    else 
    {
        _LOGTAINT(tid, insAddress, "CMPS");
        
        ObjectSource src = (isSrcTainted) 
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(esiAddr))
            : ObjectSource(lengthInBits, getMemoryValue<lengthInBits>(esiAddr));
       
        ObjectSource srcDest = (isSrcDestTainted) 
            ? ObjectSource(pTmgrGlobal->getMemoryTaint<lengthInBits>(ediAddr))
            : ObjectSource(lengthInBits, getMemoryValue<lengthInBits>(ediAddr));

        // marquage flags (resultat calcul� dans la proc�dure Flags) 
        BINARY::fTaintCMP<lengthInBits>(pTmgrTls, srcDest, src);	

        /** TODO : TRAITER LE CAS DE LA CONTRAINTE **/
        /** EN EFFET LE PREDICAT NE PORTE QUE SUR ECX/RCX : RIEN N'INDIQUE **/
        /** LA POSITION SUR LE ZFLAG. SEULE SOLUTION :TESTER EN INSTRUMENTANT **/
        /** EN IPOINT_AFTER ET TESTER LES FLAGS **/
    }
} // sCMPS