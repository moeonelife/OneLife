#include "TextField.h"

#include <string.h>

#include "minorGems/game/game.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/drawUtils.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/graphics/openGL/KeyboardHandlerGL.h"



// start:  none focused
TextField *TextField::sFocusedTextField = NULL;

int TextField::sDeleteFirstDelaySteps = 30;
int TextField::sDeleteNextDelaySteps = 2;




TextField::TextField( Font *inDisplayFont, 
                      double inX, double inY, int inCharsWide,
                      char inForceCaps,
                      const char *inLabelText,
                      const char *inAllowedChars,
                      const char *inForbiddenChars )
        : PageComponent( inX, inY ),
          mActive( true ), mFont( inDisplayFont ), 
          mCharsWide( inCharsWide ),
          mMaxLength( -1 ),
          mFireOnAnyChange( false ),
          mForceCaps( inForceCaps ),
          mLabelText( NULL ),
          mAllowedChars( NULL ), mForbiddenChars( NULL ),
          mFocused( false ), mText( new char[1] ),
          mCursorPosition( 0 ),
          mDrawnText( NULL ),
          mCursorDrawPosition( 0 ),
          mHoldDeleteSteps( -1 ), mFirstDeleteRepeatDone( false ) {
    
    if( inLabelText != NULL ) {
        mLabelText = stringDuplicate( inLabelText );
        }
    
    if( inAllowedChars != NULL ) {
        mAllowedChars = stringDuplicate( inAllowedChars );
        }
    if( inForbiddenChars != NULL ) {
        mForbiddenChars = stringDuplicate( inForbiddenChars );
        }

    clearArrowRepeat();
        

    mCharWidth = mFont->getFontHeight();

    mBorderWide = mCharWidth * 0.25;

    mHigh = mFont->getFontHeight() + 2 * mBorderWide;

    char *fullString = new char[ mCharsWide + 1 ];

    unsigned char widestChar = 0;
    double width = 0;

    for( int c=32; c<256; c++ ) {
        unsigned char pc = processCharacter( c );

        if( pc != 0 ) {
            char s[2];
            s[0] = pc;
            s[1] = '\0';

            double thisWidth = mFont->measureString( s );
            
            if( thisWidth > width ) {
                width = thisWidth;
                widestChar = pc;    
                }
            }
        }
    
    


    for( int i=0; i<mCharsWide; i++ ) {
        fullString[i] = widestChar;
        }
    fullString[ mCharsWide ] = '\0';
    
    double fullStringWidth = mFont->measureString( fullString );

    delete [] fullString;

    mWide = fullStringWidth + 2 * mBorderWide;
    
    mDrawnTextX = - ( mWide / 2 - mBorderWide );

    mText[0] = '\0';
    }



TextField::~TextField() {
    if( this == sFocusedTextField ) {
        // we're focused, now nothing is focused
        sFocusedTextField = NULL;
        }

    delete [] mText;

    if( mLabelText != NULL ) {
        delete [] mLabelText;
        }

    if( mAllowedChars != NULL ) {
        delete [] mAllowedChars;
        }
    if( mForbiddenChars != NULL ) {
        delete [] mForbiddenChars;
        }

    if( mDrawnText != NULL ) {
        delete [] mDrawnText;
        }
    }



void TextField::setText( const char *inText ) {
    delete [] mText;
    
    // obeys same rules as typing (skip blocked characters)
    SimpleVector<char> filteredText;
    
    int length = strlen( inText );
    for( int i=0; i<length; i++ ) {
        unsigned char processedChar = processCharacter( inText[i] );
        
        if( processedChar != 0 ) {
            filteredText.push_back( processedChar );
            }
        }
    

    mText = filteredText.getElementString();

    mCursorPosition = strlen( mText );

    // hold-downs broken
    mHoldDeleteSteps = -1;
    mFirstDeleteRepeatDone = false;

    clearArrowRepeat();
    }



char *TextField::getText() {
    return stringDuplicate( mText );
    }



void TextField::setMaxLength( int inLimit ) {
    mMaxLength = inLimit;
    }



void TextField::setActive( char inActive ) {
    mActive = inActive;
    }



char TextField::isActive() {
    return mActive;
    }
        


void TextField::step() {

    

    if( mHoldDeleteSteps > -1 ) {
        mHoldDeleteSteps ++;

        int stepsBetween = sDeleteFirstDelaySteps;
        
        if( mFirstDeleteRepeatDone ) {
            stepsBetween = sDeleteNextDelaySteps;
            }
        
        if( mHoldDeleteSteps > stepsBetween ) {
            // delete repeat
            mHoldDeleteSteps = 0;
            mFirstDeleteRepeatDone = true;
            
            deleteHit();
            }
        }


    for( int i=0; i<2; i++ ) {
        
        if( mHoldArrowSteps[i] > -1 ) {
            mHoldArrowSteps[i] ++;

            int stepsBetween = sDeleteFirstDelaySteps;
        
            if( mFirstArrowRepeatDone[i] ) {
                stepsBetween = sDeleteNextDelaySteps;
                }
        
            if( mHoldArrowSteps[i] > stepsBetween ) {
                // arrow repeat
                mHoldArrowSteps[i] = 0;
                mFirstArrowRepeatDone[i] = true;
            
                switch( i ) {
                    case 0:
                        leftHit();
                        break;
                    case 1:
                        rightHit();
                        break;
                    }
                }
            }
        }


    }

        
        
void TextField::draw() {
    
    if( mFocused ) {    
        setDrawColor( 1, 1, 1, 1 );
        }
    else {
        setDrawColor( 0.5, 0.5, 0.5, 1 );
        }
    

    drawRect( - mWide / 2, - mHigh / 2, 
              mWide / 2, mHigh / 2 );
    
    setDrawColor( 0.25, 0.25, 0.25, 1 );
    double pixWidth = mCharWidth / 8;


    double rectStartX = - mWide / 2 + pixWidth;
    double rectStartY = - mHigh / 2 + pixWidth;

    double rectEndX = mWide / 2 - pixWidth;
    double rectEndY = mHigh / 2 - pixWidth;
    
    drawRect( rectStartX, rectStartY,
              rectEndX, rectEndY );
    
    setDrawColor( 1, 1, 1, 1 );

    
    if( mLabelText != NULL ) {
        doublePair labelPos = { - mWide/2 - mBorderWide, 0 };
        
        mFont->drawString( mLabelText, labelPos, alignRight );
        }
    

    doublePair textPos = { - mWide/2 + mBorderWide, 0 };


    char tooLongFront = false;
    char tooLongBack = false;
    
    mCursorDrawPosition = mCursorPosition;


    char *textBeforeCursorBase = stringDuplicate( mText );
    char *textAfterCursorBase = stringDuplicate( mText );
    
    char *textBeforeCursor = textBeforeCursorBase;
    char *textAfterCursor = textAfterCursorBase;

    textBeforeCursor[ mCursorPosition ] = '\0';
    
    textAfterCursor = &( textAfterCursor[ mCursorPosition ] );

    if( mFont->measureString( mText ) > mWide - 2 * mBorderWide ) {
        
        if( mFont->measureString( textBeforeCursor ) > 
            mWide / 2 - mBorderWide
            &&
            mFont->measureString( textAfterCursor ) > 
            mWide / 2 - mBorderWide ) {

            // trim both ends

            while( mFont->measureString( textBeforeCursor ) > 
                   mWide / 2 - mBorderWide ) {
                
                tooLongFront = true;
                
                textBeforeCursor = &( textBeforeCursor[1] );
                
                mCursorDrawPosition --;
                }
        
            while( mFont->measureString( textAfterCursor ) > 
                   mWide / 2 - mBorderWide ) {
                
                tooLongBack = true;
                
                textAfterCursor[ strlen( textAfterCursor ) - 1 ] = '\0';
                }
            }
        else if( mFont->measureString( textBeforeCursor ) > 
                 mWide / 2 - mBorderWide ) {

            // just trim front
            char *sumText = concatonate( textBeforeCursor, textAfterCursor );
            
            while( mFont->measureString( sumText ) > 
                   mWide - 2 * mBorderWide ) {
                
                tooLongFront = true;
                
                textBeforeCursor = &( textBeforeCursor[1] );
                
                mCursorDrawPosition --;
                
                delete [] sumText;
                sumText = concatonate( textBeforeCursor, textAfterCursor );
                }
            delete [] sumText;
            }    
        else if( mFont->measureString( textAfterCursor ) > 
                 mWide / 2 - mBorderWide ) {
            
            // just trim back
            char *sumText = concatonate( textBeforeCursor, textAfterCursor );

            while( mFont->measureString( sumText ) > 
                   mWide - 2 * mBorderWide ) {
                
                tooLongBack = true;
                
                textAfterCursor[ strlen( textAfterCursor ) - 1 ] = '\0';
                delete [] sumText;
                sumText = concatonate( textBeforeCursor, textAfterCursor );
                }
            delete [] sumText;
            }
        }

    
    if( mDrawnText != NULL ) {
        delete [] mDrawnText;
        }
    
    mDrawnText = concatonate( textBeforeCursor, textAfterCursor );

    char leftAlign = true;
    char cursorCentered = false;
    doublePair centerPos = { 0, 0 };
    
    if( ! tooLongFront ) {
        mFont->drawString( mDrawnText, textPos, alignLeft );
        mDrawnTextX = textPos.x;
        }
    else if( tooLongFront && ! tooLongBack ) {
        
        leftAlign = false;

        doublePair textPos2 = { mWide/2 - mBorderWide, 0 };

        mFont->drawString( mDrawnText, textPos2, alignRight );
        mDrawnTextX = textPos2.x - mFont->measureString( mDrawnText );
        }
    else {
        // text around perfectly centered cursor
        cursorCentered = true;
        
        double beforeLength = mFont->measureString( textBeforeCursor );
        
        double xDiff = centerPos.x - ( textPos.x + beforeLength );
        
        doublePair textPos2 = textPos;
        textPos2.x += xDiff;

        mFont->drawString( mDrawnText, textPos2, alignLeft );
        mDrawnTextX = textPos2.x;
        }
    

    if( tooLongFront ) {
        // draw shaded overlay over left of string
        
        double verts[] = { rectStartX, rectStartY,
                           rectStartX, rectEndY,
                           rectStartX + 4 * mCharWidth, rectEndY,
                           rectStartX + 4 * mCharWidth, rectStartY };
        float vertColors[] = { 0.25, 0.25, 0.25, 1,
                               0.25, 0.25, 0.25, 1,
                               0.25, 0.25, 0.25, 0,
                               0.25, 0.25, 0.25, 0 };

        drawQuads( 1, verts , vertColors );
        }
    if( tooLongBack ) {
        // draw shaded overlay over right of string
        
        double verts[] = { rectEndX - 4 * mCharWidth, rectStartY,
                           rectEndX - 4 * mCharWidth, rectEndY,
                           rectEndX, rectEndY,
                           rectEndX, rectStartY };
        float vertColors[] = { 0.25, 0.25, 0.25, 0,
                               0.25, 0.25, 0.25, 0,
                               0.25, 0.25, 0.25, 1,
                               0.25, 0.25, 0.25, 1 };

        drawQuads( 1, verts , vertColors );
        }
    
    if( mFocused && mCursorDrawPosition > -1 ) {            
        // make measurement to draw cursor

        char *beforeCursorText = stringDuplicate( mDrawnText );
        
        beforeCursorText[ mCursorDrawPosition ] = '\0';
        
        
        double cursorXOffset;

        if( cursorCentered ) {
            cursorXOffset = mWide / 2 - mBorderWide;
            }
        else if( leftAlign ) {
            cursorXOffset = mFont->measureString( textBeforeCursor );
            if( cursorXOffset == 0 ) {
                cursorXOffset -= pixWidth;
                }
            }
        else {
            double afterLength = mFont->measureString( textAfterCursor );
            cursorXOffset = ( mWide - 2 * mBorderWide ) - afterLength;

            if( afterLength > 0 ) {
                cursorXOffset -= pixWidth;
                }
            }
        

        
        delete [] beforeCursorText;
        
        setDrawColor( 0, 0, 0, 0.5 );
        
        drawRect( textPos.x + cursorXOffset, 
                  rectStartY - pixWidth,
                  textPos.x + cursorXOffset + pixWidth, 
                  rectEndY + pixWidth );
        }
    
    
    if( ! mActive ) {
        setDrawColor( 0, 0, 0, 0.5 );
        // dark overlay
        drawRect( - mWide / 2, - mHigh / 2, 
                  mWide / 2, mHigh / 2 );
        }
        

    delete [] textBeforeCursorBase;
    delete [] textAfterCursorBase;
    }


void TextField::pointerUp( float inX, float inY ) {
    if( inX > - mWide / 2 &&
        inX < + mWide / 2 &&
        inY > - mHigh / 2 &&
        inY < + mHigh / 2 ) {

        focus();

        
        int bestCursorDrawPosition = mCursorDrawPosition;
        double bestDistance = mWide * 2;
        
        int drawnTextLength = strlen( mDrawnText );
        
        // find gap between drawn letters that is closest to clicked x

        for( int i=0; i<=drawnTextLength; i++ ) {
            
            char *textCopy = stringDuplicate( mDrawnText );
            
            textCopy[i] = '\0';

            double thisGapX = 
                mDrawnTextX + 
                mFont->measureString( textCopy ) +
                mFont->getCharSpacing() / 2;
            
            delete [] textCopy;
            
            double thisDistance = fabs( thisGapX - inX );
            
            if( thisDistance < bestDistance ) {
                bestCursorDrawPosition = i;
                bestDistance = thisDistance;
                }
            }
        
        int cursorDelta = bestCursorDrawPosition - mCursorDrawPosition;
        
        mCursorPosition += cursorDelta;
        }
    }




unsigned char TextField::processCharacter( unsigned char inASCII ) {

    unsigned char processedChar = inASCII;
        
    if( mForceCaps ) {
        processedChar = toupper( inASCII );
        }

    if( mForbiddenChars != NULL ) {
        int num = strlen( mForbiddenChars );
            
        for( int i=0; i<num; i++ ) {
            if( mForbiddenChars[i] == processedChar ) {
                return 0;
                }
            }
        }
        

    if( mAllowedChars != NULL ) {
        int num = strlen( mAllowedChars );
            
        char allowed = false;
            
        for( int i=0; i<num; i++ ) {
            if( mAllowedChars[i] == processedChar ) {
                allowed = true;
                break;
                }
            }

        if( !allowed ) {
            return 0;
            }
        }

    return processedChar;
    }



void TextField::insertCharacter( unsigned char inASCII ) {
    
    // add to it
    char *oldText = mText;
    
    if( mMaxLength != -1 &&
        strlen( oldText ) >= (unsigned int) mMaxLength ) {
        // max length hit, don't add it
        return;
        }
    

    char *preCursor = stringDuplicate( mText );
    preCursor[ mCursorPosition ] = '\0';
    char *postCursor = &( mText[ mCursorPosition ] );
    
    mText = autoSprintf( "%s%c%s", 
                         preCursor, inASCII, postCursor );
    
    delete [] preCursor;
    
    delete [] oldText;
    
    mCursorPosition++;
    }



void TextField::cursorReset() {
    mCursorPosition = 0;
    }



double TextField::getRightEdgeX() {
    
    return mX + mWide / 2;
    }



void TextField::setFireOnAnyTextChange( char inFireOnAny ) {
    mFireOnAnyChange = inFireOnAny;
    }




void TextField::keyDown( unsigned char inASCII ) {
    if( !mFocused ) {
        return;
        }
    
    if( isCommandKeyDown() ) {
        // not a normal key stroke (command key)
        // ignore it as input

        // but ONLY if it's an alphabetical key (A-Z,a-z)
        // Some international keyboards use ALT to type certain symbols

        if( ( inASCII >= 'A' && inASCII <= 'Z' )
            ||
            ( inASCII >= 'a' && inASCII <= 'z' ) ) {
            
            return;
            }
        
        }
    

    if( inASCII == 127 || inASCII == 8 ) {
        // delete
        deleteHit();
        
        mHoldDeleteSteps = 0;

        clearArrowRepeat();
        }
    else if( inASCII == 13 ) {
        // enter hit in field
        fireActionPerformed( this );
        }
    else if( inASCII >= 32 ) {

        unsigned char processedChar = processCharacter( inASCII );    

        if( processedChar != 0 ) {
            
            insertCharacter( processedChar );
            }
        
        mHoldDeleteSteps = -1;
        mFirstDeleteRepeatDone = false;

        clearArrowRepeat();

        if( mFireOnAnyChange ) {
            fireActionPerformed( this );
            }
        }    
    }



void TextField::keyUp( unsigned char inASCII ) {
    if( inASCII == 127 || inASCII == 8 ) {
        // end delete hold down
        mHoldDeleteSteps = -1;
        mFirstDeleteRepeatDone = false;
        }
    }



void TextField::deleteHit() {
    if( mCursorPosition > 0 ) {

        char *oldText = mText;
        
        char *preCursor = stringDuplicate( mText );
        preCursor[ mCursorPosition - 1 ] = '\0';
        char *postCursor = &( mText[ mCursorPosition ] );

        mText = autoSprintf( "%s%s", preCursor, postCursor );
        
        delete [] preCursor;

        delete [] oldText;

        mCursorPosition--;

        if( mFireOnAnyChange ) {
            fireActionPerformed( this );
            }
        }
    }



void TextField::clearArrowRepeat() {
    for( int i=0; i<2; i++ ) {
        mHoldArrowSteps[i] = -1;
        mFirstArrowRepeatDone[i] = false;
        }
    }



void TextField::leftHit() {
    mCursorPosition --;
    if( mCursorPosition < 0 ) {
        mCursorPosition = 0;
        }
    }



void TextField::rightHit() {
    mCursorPosition ++;
    if( mCursorPosition > (int)strlen( mText ) ) {
        mCursorPosition = strlen( mText );
        }
    }




void TextField::specialKeyDown( int inKeyCode ) {
    if( !mFocused ) {
        return;
        }
    
    switch( inKeyCode ) {
        case MG_KEY_LEFT:
            leftHit();
            clearArrowRepeat();
            mHoldArrowSteps[0] = 0;
            break;
        case MG_KEY_RIGHT:
            rightHit(); 
            clearArrowRepeat();
            mHoldArrowSteps[1] = 0;
            break;
        default:
            break;
        }
    
    }



void TextField::specialKeyUp( int inKeyCode ) {
    if( inKeyCode == MG_KEY_LEFT ) {
        mHoldArrowSteps[0] = -1;
        mFirstArrowRepeatDone[0] = false;
        }
    else if( inKeyCode == MG_KEY_RIGHT ) {
        mHoldArrowSteps[1] = -1;
        mFirstArrowRepeatDone[1] = false;
        }
    }



void TextField::focus() {
    
    if( sFocusedTextField != NULL ) {
        // unfocus last focused
        sFocusedTextField->unfocus();
        }

    mFocused = true;
    sFocusedTextField = this;
    }



void TextField::unfocus() {
    mFocused = false;
 
    // hold-down broken if not focused
    mHoldDeleteSteps = -1;
    mFirstDeleteRepeatDone = false;

    clearArrowRepeat();

    if( sFocusedTextField == this ) {
        sFocusedTextField = NULL;
        }    
    }



char TextField::isFocused() {
    return mFocused;
    }



void TextField::setDeleteRepeatDelays( int inFirstDelaySteps,
                                       int inNextDelaySteps ) {
    sDeleteFirstDelaySteps = inFirstDelaySteps;
    sDeleteNextDelaySteps = inNextDelaySteps;
    }
