/* This file is part of the KDE project

   Note: Mostly taken from Daap code:
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef MTPMETA_H
#define MTPMETA_H

#include "Debug.h"
#include "Meta.h"

#include <libmtp.h>

#include <QMultiMap>

class MtpCollection;
class PopupDropperAction;

namespace Meta
{

class MtpTrack;
class MtpAlbum;
class MtpArtist;
class MtpGenre;
class MtpComposer;
class MtpYear;

typedef KSharedPtr<MtpTrack> MtpTrackPtr;
typedef KSharedPtr<MtpArtist> MtpArtistPtr;
typedef KSharedPtr<MtpAlbum> MtpAlbumPtr;
typedef KSharedPtr<MtpGenre> MtpGenrePtr;
typedef KSharedPtr<MtpComposer> MtpComposerPtr;
typedef KSharedPtr<MtpYear> MtpYearPtr;

typedef QMultiMap<MtpArtistPtr, MtpTrackPtr> MtpArtistMap;
typedef QMultiMap<MtpAlbumPtr, MtpTrackPtr> MtpAlbumMap;
typedef QMultiMap<MtpGenrePtr, MtpTrackPtr> MtpGenreMap;
typedef QMultiMap<MtpComposerPtr, MtpTrackPtr> MtpComposerMap;
typedef QMultiMap<MtpYearPtr, MtpTrackPtr> MtpYearMap;

class MtpTrack : public Meta::Track
{

    public:
        MtpTrack( MtpCollection *collection, const QString &format);
        virtual ~MtpTrack();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual KUrl playableUrl() const;
        virtual QString url() const;
        virtual QString prettyUrl() const;

        virtual bool isPlayable() const;
        virtual bool isEditable() const;

        virtual AlbumPtr album() const;
        virtual ArtistPtr artist() const;
        virtual GenrePtr genre() const;
        virtual ComposerPtr composer() const;
        virtual YearPtr year() const;

        LIBMTP_track_t* getMtpTrack() const;
        void setMtpTrack ( LIBMTP_track_t *mtptrack );

        // TODO: add mtp-type playlist functions
/*
	QList<MTP_Playlist*> getMtpPlaylists() const;
	void addMtpPlaylist ( MTP_Playlist *mtpplaylist );

    */

        virtual void setAlbum ( const QString &newAlbum );
        virtual void setArtist ( const QString &newArtist );
        virtual void setGenre ( const QString &newGenre );
        virtual void setComposer ( const QString &newComposer );
        virtual void setYear ( const QString &newYear );

        virtual QString title() const;
        virtual void setTitle( const QString &newTitle );

        virtual QString comment() const;
        virtual void setComment ( const QString &newComment );

        virtual double score() const;
        virtual void setScore ( double newScore );

        virtual int rating() const;
        virtual void setRating ( int newRating );

        virtual int length() const;

        void setFileSize( int newFileSize );
        virtual int filesize() const;
        virtual int sampleRate() const;

        virtual int bitrate() const;
        virtual void setBitrate( int newBitrate );

        virtual int samplerate() const;
        virtual void setSamplerate( int newSamplerate );

        virtual float bpm() const;
        virtual void setBpm( float newBpm );

        virtual int trackNumber() const;
        virtual void setTrackNumber ( int newTrackNumber );

        virtual int discNumber() const;
        virtual void setDiscNumber ( int newDiscNumber );

        virtual uint lastPlayed() const;
        virtual int playCount() const;

        virtual QString type() const;

        virtual void beginMetaDataUpdate() { DEBUG_BLOCK }    //read only
        virtual void endMetaDataUpdate();
        virtual void abortMetaDataUpdate() { DEBUG_BLOCK }    //read only

        virtual void subscribe ( Observer *observer );
        virtual void unsubscribe ( Observer *observer );

        virtual bool inCollection() const;
        virtual Collection* collection() const;

	virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;
	virtual Meta::Capability* asCapabilityInterface( Meta::Capability::Type type );

        //MtpTrack specific methods
    
    public:
        // These methods are for MemoryMatcher to use
        void setAlbum( MtpAlbumPtr album );
        void setArtist( MtpArtistPtr artist );
        void setComposer( MtpComposerPtr composer );
        void setGenre( MtpGenrePtr genre );
        void setYear( MtpYearPtr year );

        // These methods are for MtpTrack-specific usage
        // NOTE: these methods/data may turn out to be unneeded
        MtpArtistMap getMtpArtistMap() const { return m_mtpArtistMap; }
        MtpAlbumMap getMtpAlbumMap() const { return m_mtpAlbumMap; }
        MtpGenreMap getMtpGenreMap() const { return m_mtpGenreMap; }
        MtpComposerMap getMtpComposerMap() const { return m_mtpComposerMap; }
        MtpYearMap getMtpYearMap() const { return m_mtpYearMap; }

        void setMtpArtistMap( const MtpArtistMap &mtpArtistMap ) { m_mtpArtistMap = mtpArtistMap; }
        void setMtpAlbumMap( const MtpAlbumMap &mtpAlbumMap ) { m_mtpAlbumMap = mtpAlbumMap; }
        void setMtpGenreMap( const MtpGenreMap &mtpGenreMap ) { m_mtpGenreMap = mtpGenreMap; }
        void setMtpComposerMap( const MtpComposerMap &mtpComposerMap ) { m_mtpComposerMap = mtpComposerMap; }
        void setMtpYearMap( const MtpYearMap &mtpYearMap ) { m_mtpYearMap = mtpYearMap; }

    
        void setLength( int length );
	void setPlayableUrl( QString Url ) { m_playableUrl = Url; }

    private:
        MtpCollection *m_collection;

        MtpArtistPtr m_artist;
        MtpAlbumPtr m_album;
        MtpGenrePtr m_genre;
        MtpComposerPtr m_composer;
        MtpYearPtr m_year;

        // For MtpTrack-specific use

        MtpArtistMap m_mtpArtistMap;
        MtpAlbumMap m_mtpAlbumMap;
        MtpGenreMap m_mtpGenreMap;
        MtpComposerMap m_mtpComposerMap;
        MtpYearMap m_mtpYearMap;

        LIBMTP_track_t *m_mtptrack;
//	QList<MTP_Playlist*> m_mtpplaylists;

        QString m_comment;
        QString m_name;
        QString m_type;
        int m_bitrate;
        int m_filesize;
        int m_length;
        int m_discNumber;
        int m_samplerate;
        int m_trackNumber;
        float m_bpm;
        QString m_displayUrl;
        QString m_playableUrl;
};

class MtpArtist : public Meta::Artist
{
    public:
        MtpArtist( const QString &name );
        virtual ~MtpArtist();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        virtual AlbumList albums();

        //MtpArtist specific methods
        void addTrack( MtpTrackPtr track );
        void remTrack( MtpTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class MtpAlbum : public Meta::Album
{
    public:
        MtpAlbum( const QString &name );
        virtual ~MtpAlbum();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual bool isCompilation() const;
        virtual bool hasAlbumArtist() const;
        virtual ArtistPtr albumArtist() const;
        virtual TrackList tracks();

        virtual QPixmap image( int size = 1, bool withShadow = false );
        virtual bool canUpdateImage() const;
        virtual void setImage( const QImage &image);

        //MtpAlbum specific methods
        void addTrack( MtpTrackPtr track );
        void remTrack( MtpTrackPtr track );
        void setAlbumArtist( MtpArtistPtr artist );
        void setIsCompilation( bool compilation );

    private:
        QString m_name;
        TrackList m_tracks;
        bool m_isCompilation;
        MtpArtistPtr m_albumArtist;
};

class MtpGenre : public Meta::Genre
{
    public:
        MtpGenre( const QString &name );
        virtual ~MtpGenre();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //MtpGenre specific methods
        void addTrack( MtpTrackPtr track );
        void remTrack( MtpTrackPtr track );
    private:
        QString m_name;
        TrackList m_tracks;
};

class MtpComposer : public Meta::Composer
{
    public:
        MtpComposer( const QString &name );
        virtual ~MtpComposer();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //MtpComposer specific methods
        void addTrack( MtpTrackPtr track );
        void remTrack( MtpTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class MtpYear : public Meta::Year
{
    public:
        MtpYear( const QString &name );
        virtual ~MtpYear();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //MtpYear specific methods
        void addTrack( MtpTrackPtr track );
        void remTrack( MtpTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

}

#endif

