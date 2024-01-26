using Godot;
using System;

public partial class node_2d : Node2D
{
    [Export]
    FastNoiseLite? noise;

    override public void _Process(double delta)
    {
        if (noise != null)
        {
            float sum = 0;
            for (int i = 0; i < 1; i++)
            {
                ulong t = Time.GetTicksMsec();

                for (int y = 0; y < 1000; y++)
                {
                    for (int x = 0; x < 1000; x++)
                    {
                        sum += noise.GetNoise2D(x, y);
                    }
                }

                GD.Print(Time.GetTicksMsec() - t);
            }

            // GD.Print(sum);
        }
    }
}
