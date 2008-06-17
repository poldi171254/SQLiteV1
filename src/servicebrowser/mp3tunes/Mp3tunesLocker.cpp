/***************************************************************************
 *   Copyright (c) 2007  Casey Link <unnamedrambler@gmail.com>             *
 *                                                                         *
 *   This program is free software{} you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation{} either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY{} without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program{} if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "Mp3tunesLocker.h"

#include "Debug.h"

#include <QByteArray>


Mp3tunesLocker::Mp3tunesLocker( const QString & partnerToken)
{
    DEBUG_BLOCK
    debug() << "New Locker Wrapper";
    QByteArray ba = partnerToken.toLatin1();
    const char *c_tok = ba.data();
    debug() << "Wrapper Token: " << c_tok;
    mp3tunes_locker_init(&mp3tunes_locker, const_cast<char*>(c_tok) );
}

Mp3tunesLocker::Mp3tunesLocker( const QString & partnerToken, const QString & userName, const QString & password)
{
    QByteArray ba = partnerToken.toLatin1();
    const char *c_tok = ba.data();
    mp3tunes_locker_init(&mp3tunes_locker, const_cast<char*>(c_tok) );

    this->login( userName, password );
}

Mp3tunesLocker::~Mp3tunesLocker(){
    mp3tunes_locker_deinit(&mp3tunes_locker);
}

QString Mp3tunesLocker::login( const QString & userName, const QString & password )
{
    DEBUG_BLOCK
    QByteArray baUser = userName.toLatin1();
    const char *c_user = baUser.data();

    QByteArray baPass = password.toLatin1();
    const char *c_pass = baPass.data();

    //result = 0 Login successful
    //result != 0 Login failed
    debug() << "Wrapper Logging on with: " << userName << ":" << password;
    int result = mp3tunes_locker_login(mp3tunes_locker, const_cast<char*>(c_user),  const_cast<char*>(c_pass) );

    if(result == 0) { //login successful
        debug() << "Wrapper Login succeeded. result: " << result;
        return this->sessionId();
    }
    debug() << "Wrapper Login failed. result: " << result;
    return QString(); //login failed
}

bool Mp3tunesLocker::sessionValid() const
{
    //result = 0 session valid
    //result != 0 session invalid
    int result = mp3tunes_locker_session_valid(mp3tunes_locker);
    if(result == 0)
    {
        return true;
    }
    return false;
}

QList<Mp3tunesLockerArtist> Mp3tunesLocker::artists() const
{
    DEBUG_BLOCK
    QList<Mp3tunesLockerArtist> artistsQList; // to be returned
    mp3tunes_locker_artist_list_t *artists_list;
    mp3tunes_locker_list_item_t *artist_item;
     // the value holder

    //get the list of artists
    mp3tunes_locker_artists(mp3tunes_locker, &artists_list);

    artist_item = artists_list->first; // the current node

    //looping through the list of artists
    while (artist_item != 0) {
        // get the artist from the c lib
        mp3tunes_locker_artist_t *artist = (mp3tunes_locker_artist_t*)artist_item->value;
        //debug() << "Wrapper Artist: " << &artist->artistName << " " << artist->artistName;

        //wrap it up
        Mp3tunesLockerArtist artistWrapped(artist);
        //and stick it in the QList
        artistsQList.append( artistWrapped );
        //advance to next artist
        artist_item = artist_item->next;
    }
    mp3tunes_locker_artist_list_deinit(&artists_list);
   // debug() << "Wrapper deinit Complete";
    return artistsQList;
}

QList<Mp3tunesLockerArtist> Mp3tunesLocker::artistsSearch( const QString &query ) const
{
    DEBUG_BLOCK
    Mp3tunesSearchResult container;
    container.searchFor = Mp3tunesSearchResult::ArtistQuery;
    search(container, query);
    return container.artistList;
}

QList<Mp3tunesLockerAlbum> Mp3tunesLocker::albums() const
{
    QList<Mp3tunesLockerAlbum> albumsQList; // to be returned
    mp3tunes_locker_album_list_t *albums_list;
    mp3tunes_locker_list_item_t *album_item;

    //get the list of albums
    mp3tunes_locker_albums(mp3tunes_locker, &albums_list);

    mp3tunes_locker_album_t *album; // the value holder
    album_item = albums_list->first; // the current node

    //looping through the list of albums
    while (album_item != NULL) {
        // get the album from the c lib
        album = (mp3tunes_locker_album_t*)album_item->value;
        //wrap it up
        Mp3tunesLockerAlbum albumWrapped(album);
        //and stick it in the QList
        albumsQList.append( albumWrapped );
        //advance to next album
        album_item = album_item->next;
    }
    mp3tunes_locker_album_list_deinit(&albums_list);

    return albumsQList;
}

QList<Mp3tunesLockerAlbum> Mp3tunesLocker::albumsSearch( const QString &query ) const
{
    Mp3tunesSearchResult container;
    container.searchFor = Mp3tunesSearchResult::AlbumQuery;
    search(container, query);
    return container.albumList;
}
QList<Mp3tunesLockerAlbum> Mp3tunesLocker::albumsWithArtistId( int artistId ) const
{
    QList<Mp3tunesLockerAlbum> albumsQList; // to be returned
    mp3tunes_locker_album_list_t *albums_list;
    mp3tunes_locker_list_item_t *album_item;

    //get the list of albums
    mp3tunes_locker_albums_with_artist_id(mp3tunes_locker, &albums_list, artistId);

    mp3tunes_locker_album_t *album; // the value holder
    album_item = albums_list->first; // the current node

    //looping through the list of albums
    while (album_item != NULL) {
        // get the album from the c lib
        album = (mp3tunes_locker_album_t*)album_item->value;
        //wrap it up
        Mp3tunesLockerAlbum albumWrapped(album);
        //and stick it in the QList
        albumsQList.append( albumWrapped );
        //advance to next album
        album_item = album_item->next;
    }
    mp3tunes_locker_album_list_deinit(&albums_list);

    return albumsQList;
}

QList<Mp3tunesLockerPlaylist> Mp3tunesLocker::playlists() const
{
    QList<Mp3tunesLockerPlaylist> playlistsQList; // to be returned

    mp3tunes_locker_playlist_list_t *playlist_list;
    mp3tunes_locker_list_item_t *playlist_item;
    mp3tunes_locker_playlist_t *playlist;

    mp3tunes_locker_playlists(this->mp3tunes_locker, &playlist_list);

    playlist_item = playlist_list->first;
    while (playlist_item != NULL) {
        playlist = (mp3tunes_locker_playlist_t*)playlist_item->value;

        Mp3tunesLockerPlaylist playlistWrapped(playlist);
        playlistsQList.append(playlistWrapped);

        playlist_item = playlist_item->next;
    }
    mp3tunes_locker_playlist_list_deinit(&playlist_list);

    return playlistsQList;
}

QList<Mp3tunesLockerTrack> Mp3tunesLocker::tracks() const
{
    QList<Mp3tunesLockerTrack> tracksQList; // to be returned

    mp3tunes_locker_track_list_t *tracks_list;
    mp3tunes_locker_list_item_t *track_item;
    mp3tunes_locker_track_t *track;

    mp3tunes_locker_tracks(mp3tunes_locker, &tracks_list);

    track_item = tracks_list->first;
    while (track_item != NULL) {
        track = (mp3tunes_locker_track_t*)track_item->value;

        Mp3tunesLockerTrack trackWrapped(track);
        tracksQList.append(trackWrapped);

        track_item = track_item->next;
    }
    mp3tunes_locker_track_list_deinit(&tracks_list);

    return tracksQList;
}

QList<Mp3tunesLockerTrack> Mp3tunesLocker::tracksSearch( const QString &query ) const
{
    Mp3tunesSearchResult container;
    container.searchFor = Mp3tunesSearchResult::TrackQuery;
    search(container, query);
    return container.trackList;
}
QList<Mp3tunesLockerTrack> Mp3tunesLocker::tracksWithPlaylistId( const QString & playlistId ) const
{
    //convert Qstring to char*
    QByteArray baStr = playlistId.toLatin1();
    const char *cc_str = baStr.data();
    char* c_playlistid = const_cast<char*>(cc_str);

    QList<Mp3tunesLockerTrack> tracksQList; // to be returned

    mp3tunes_locker_track_list_t *tracks_list;
    mp3tunes_locker_list_item_t *track_item;
    mp3tunes_locker_track_t *track;

    mp3tunes_locker_tracks_with_playlist_id(mp3tunes_locker, &tracks_list, c_playlistid);

    track_item = tracks_list->first;
    while (track_item != NULL) {
        track = (mp3tunes_locker_track_t*)track_item->value;

        Mp3tunesLockerTrack trackWrapped(track);
        tracksQList.append(trackWrapped);

        track_item = track_item->next;
    }
    mp3tunes_locker_track_list_deinit(&tracks_list);

    return tracksQList;
}

QList<Mp3tunesLockerTrack> Mp3tunesLocker::tracksWithAlbumId( int albumId ) const
{
    QList<Mp3tunesLockerTrack> tracksQList; // to be returned

    mp3tunes_locker_track_list_t *tracks_list;
    mp3tunes_locker_list_item_t *track_item;
    mp3tunes_locker_track_t *track;

    mp3tunes_locker_tracks_with_album_id(mp3tunes_locker, &tracks_list, albumId);

    track_item = tracks_list->first;
    while (track_item != NULL) {
        track = (mp3tunes_locker_track_t*)track_item->value;

        Mp3tunesLockerTrack trackWrapped(track);
        tracksQList.append(trackWrapped);

        track_item = track_item->next;
    }
    mp3tunes_locker_track_list_deinit(&tracks_list);

    return tracksQList;
}

QList<Mp3tunesLockerTrack> Mp3tunesLocker::tracksWithArtistId( int artistId ) const
{
    QList<Mp3tunesLockerTrack> tracksQList; // to be returned

    mp3tunes_locker_track_list_t *tracks_list;
    mp3tunes_locker_list_item_t *track_item;
    mp3tunes_locker_track_t *track;

    mp3tunes_locker_tracks_with_artist_id(mp3tunes_locker, &tracks_list, artistId);

    track_item = tracks_list->first;
    while (track_item != NULL) {
        track = (mp3tunes_locker_track_t*)track_item->value;

        Mp3tunesLockerTrack trackWrapped(track);
        tracksQList.append(trackWrapped);

        track_item = track_item->next;
    }
    mp3tunes_locker_track_list_deinit(&tracks_list);

    return tracksQList;
}

bool Mp3tunesLocker::search( Mp3tunesSearchResult &container, const QString &query ) const
{

    // setup vars
    mp3tunes_locker_artist_list_t *artists_list;
    mp3tunes_locker_list_item_t *artist_item;
    mp3tunes_locker_artist_t *artist;

    mp3tunes_locker_album_list_t *albums_list;
    mp3tunes_locker_list_item_t *album_item;
    mp3tunes_locker_album_t *album;

    mp3tunes_locker_track_list_t *tracks_list;
    mp3tunes_locker_list_item_t *track_item;
    mp3tunes_locker_track_t *track;

    if( container.searchFor && Mp3tunesSearchResult::ArtistQuery )
    {
        artists_list = 0;
    }
    if( container.searchFor && Mp3tunesSearchResult::AlbumQuery )
    {
        albums_list = 0;
    }
    if( container.searchFor && Mp3tunesSearchResult::TrackQuery )
    {
        tracks_list = 0;
    }

    //convert Qstring to char*
    QByteArray baStr = query.toLatin1();
    const char *cc_str = baStr.data();
    char* c_query = const_cast<char*>(cc_str);

    int res = mp3tunes_locker_search(mp3tunes_locker, &artists_list, &albums_list, &tracks_list, c_query);
    if(res != 0)
    {
        return false;
    }
    if( container.searchFor && Mp3tunesSearchResult::ArtistQuery )
    {
        artist_item = artists_list->first;
        while (artist_item != NULL) {
            artist = (mp3tunes_locker_artist_t*)artist_item->value;

            Mp3tunesLockerArtist artistWrapped(artist);
            container.artistList.append(artistWrapped);

            artist_item = artist_item->next;
        }
        mp3tunes_locker_artist_list_deinit(&artists_list);
    }

    if( container.searchFor && Mp3tunesSearchResult::AlbumQuery )
    {
        album_item = albums_list->first;
        while (album_item != NULL) {
            album = (mp3tunes_locker_album_t*)album_item->value;

            Mp3tunesLockerAlbum albumWrapped(album);
            container.albumList.append(albumWrapped);

            album_item = album_item->next;
        }
        mp3tunes_locker_album_list_deinit(&albums_list);
    }

    if( container.searchFor && Mp3tunesSearchResult::TrackQuery )
    {
        track_item = tracks_list->first;
        while (track_item != NULL) {
            track = (mp3tunes_locker_track_t*)track_item->value;

            Mp3tunesLockerTrack trackWrapped(track);
            container.trackList.append(trackWrapped);

            track_item = track_item->next;
        }
        mp3tunes_locker_track_list_deinit(&tracks_list);
    }
    return true;
}

bool Mp3tunesLocker::uploadTrack( const QString &path )
{
    //convert Qstring to char*
    QByteArray baStr = path.toLatin1();
    const char *cc_str = baStr.data();
    char* c_path = const_cast<char*>(cc_str);

    int res = mp3tunes_locker_upload_track(mp3tunes_locker, c_path);
    if(res == 0) {
        return true;
    }
    return false;
}

QString Mp3tunesLocker::fileKey(const QString &path )
{
    //convert Qstring to char*
    QByteArray baStr = path.toLatin1();
    const char *cc_str = baStr.data();
    char* c_path = const_cast<char*>(cc_str);

    char* file_key = (char*)malloc(4096*sizeof(char));
    file_key = mp3tunes_locker_generate_filekey(c_path);

    return QString(file_key);
}

QString Mp3tunesLocker::userName() const
{
    return QString( mp3tunes_locker->username );
}

QString Mp3tunesLocker::password() const
{
    return QString( mp3tunes_locker->password );
}

QString Mp3tunesLocker::sessionId() const
{
    return QString( mp3tunes_locker->session_id );
}

QString Mp3tunesLocker::firstName() const
{
    return QString( mp3tunes_locker->firstname );
}

QString Mp3tunesLocker::lastName() const
{
    return QString( mp3tunes_locker->lastname );
}

QString Mp3tunesLocker::nickName() const
{
    return QString( mp3tunes_locker->nickname );
}

QString Mp3tunesLocker::partnerToken() const
{
    return QString( mp3tunes_locker->partner_token );
}

QString Mp3tunesLocker::serverApi() const
{
    return QString( mp3tunes_locker->server_api );
}

QString Mp3tunesLocker::serverContent() const
{
    return QString( mp3tunes_locker->server_content );
}

QString Mp3tunesLocker::serverLogin() const
{
    return QString( mp3tunes_locker->server_login );
}
