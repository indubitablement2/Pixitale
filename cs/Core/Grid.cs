using System;
using System.Collections.Generic;
using Godot;

namespace Pixitale;

public class Grid
{
    public readonly Rect2I Rect;
    Chunk[] chunks;

    HashSet<Vector2I> activeChunks;

    Grid(int halfSize = 16)
    {
        halfSize = Util.SignedNextPowerOfTwo(Math.Max(1, halfSize));

        Rect = new Rect2I(-halfSize, -halfSize, halfSize * 2, halfSize * 2);
        chunks = new Chunk[Rect.Area];
        activeChunks = new HashSet<Vector2I>();
    }

    // void Grow(int size)
    // {
    //     var newCells = new Chunk[chunks.Length * 2];
    //     chunks.CopyTo(newCells, 0);
    //     chunks = newCells;
    // }

    void AllocateRect(Rect2I wishRect)
    {
        int newX = Util.SignedNextPowerOfTwo(wishRect.Position.X);
        int newY = Util.SignedNextPowerOfTwo(wishRect.Position.Y);
        int newEndX = Util.SignedNextPowerOfTwo(wishRect.End.X);
        int newEndY = Util.SignedNextPowerOfTwo(wishRect.End.Y);
        int newWidth = newEndX - newX;
        int newHeight = newEndY - newY;
        wishRect = new Rect2I(newX, newY, newWidth, newHeight);

        if (Rect == wishRect) return;

        GD.Print("Increasing grid size from ", Rect, " to ", wishRect);

        Chunk[] newChunks = chunks;
        if (chunks.Length < wishRect.Area)
        {
            var newCells = new Chunk[wishRect.Area];
            chunks.CopyTo(newCells, 0);
            chunks = newCells;
        }

        int currentArea = Rect.Area;

    }
}


public struct Chunk
{
    public uint[] Cells;
    /// <summary>
    /// Bitfield of rows which have at least one active cell.
    /// </summary>
    public uint ActiveRows;
    /// <summary>
    /// Bitfield of columns which have at least one active cell.
    /// </summary>
    public uint ActiveColumns;
}